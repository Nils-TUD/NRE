/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <kobj/LocalThread.h>
#include <kobj/Pt.h>
#include <kobj/Sm.h>
#include <kobj/UserSm.h>
#include <ipc/ServiceCPUHandler.h>
#include <ipc/ServiceSession.h>
#include <mem/DataSpace.h>
#include <utcb/UtcbFrame.h>
#include <Exception.h>
#include <util/ScopedPtr.h>
#include <util/CPUSet.h>
#include <bits/BitField.h>
#include <RCU.h>
#include <CPU.h>
#include <util/Math.h>

namespace nre {

template<class T>
class SessionIterator;

/**
 * The exception that is used for services
 */
class ServiceException : public Exception {
public:
    explicit ServiceException(ErrorCode code = E_FAILURE, const String &msg = String()) throw()
        : Exception(code, msg) {
    }
};

/**
 * This class is used to provide a service for clients. If you create an instance of it, it is
 * registered at your parent with a specified name and services portals, that the client can call
 * to open and close sessions. Sessions are used to bind data to them. Note that sessions are
 * always used (also if no additional data is needed) to prevent a special case.
 * As soon as a client has a session, it can use the service that is provided. That is, it can call
 * the portals.
 */
class Service {
    friend class ServiceCPUHandler;
    template<class T>
    friend class SessionIterator;

public:
    static const uint MAX_SESSIONS_ORDER        =   6;
    static const size_t MAX_SESSIONS            =   1 << MAX_SESSIONS_ORDER;

    /**
     * The commands the parent provides for working with services
     */
    enum Command {
        REGISTER,
        GET,
        UNREGISTER,
        CLIENT_DIED
    };

    /**
     * Constructor. Creates portals on the specified CPUs to accept client sessions and creates
     * Threads to handle <portal>. Note that you have to call reg() afterwards to finally register
     * the service.
     *
     * @param name the name of the service
     * @param cpus the CPUs on which you want to provide the service
     * @param portal the portal-function to provide
     */
    explicit Service(const char *name, const CPUSet &cpus, Pt::portal_func portal)
        : _regcaps(CapSelSpace::get().allocate(1 << CPU::order(), 1 << CPU::order())),
          _caps(CapSelSpace::get().allocate(MAX_SESSIONS << CPU::order(), MAX_SESSIONS << CPU::order())),
          _sm(), _kill_sm(), _stop(false), _name(name), _func(portal),
          _insts(new ServiceCPUHandler *[CPU::count()]), _reg_cpus(cpus.get()), _sessions() {
        for(size_t i = 0; i < CPU::count(); ++i) {
            if(_reg_cpus.is_set(i))
                _insts[i] = new ServiceCPUHandler(this, _regcaps + i, i);
            else
                _insts[i] = nullptr;
        }
    }
    /**
     * Destroys this service, i.e. destroys all sessions. You should have called unreg() before.
     */
    virtual ~Service() {
        for(size_t i = 0; i < MAX_SESSIONS; ++i) {
            ServiceSession *sess = rcu_dereference(_sessions[i]);
            if(sess)
                remove_session(sess);
        }
        for(size_t i = 0; i < CPU::count(); ++i)
            delete _insts[i];
        delete[] _insts;
        CapSelSpace::get().free(_caps, MAX_SESSIONS << CPU::order());
        CapSelSpace::get().free(_regcaps, 1 << CPU::order());
    }

    /**
     * Registers and starts the service
     */
    void start() {
        _stop = false;
        reg();
        while(!_stop) {
            _kill_sm->down();
            check_sessions();
        }
        unreg();
    }

    /**
     * Requests a stop of the service loop
     */
    void stop() {
        _stop = true;
        Sync::memory_barrier();
        _kill_sm->up();
    }

    /**
     * @return the service name
     */
    const char *name() const {
        return _name;
    }
    /**
     * @return the portal-function
     */
    Pt::portal_func portal() const {
        return _func;
    }
    /**
     * @return the capabilities used for all session-portals
     */
    capsel_t caps() const {
        return _caps;
    }
    /**
     * @return the bitmask that specified on which CPUs it is available
     */
    const BitField<Hip::MAX_CPUS> &available() const {
        return _reg_cpus;
    }

    /**
     * @return the iterator-beginning to walk over all sessions (note that you need to use an
     *  RCULock to prevent that sessions are destroyed while iterating over them)
     */
    template<class T>
    SessionIterator<T> sessions_begin();
    /**
     * @return the iterator-end
     */
    template<class T>
    SessionIterator<T> sessions_end();

    /**
     * @param pid the portal-selector
     * @return the session
     * @throws ServiceException if the session does not exist
     */
    template<class T>
    T *get_session(capsel_t pid) {
        return get_session_by_id<T>((pid - _caps) >> CPU::order());
    }
    /**
     * @param id the session-id
     * @return the session
     * @throws ServiceException if the session does not exist
     */
    template<class T>
    T *get_session_by_id(size_t id) {
        T *sess = static_cast<T*>(rcu_dereference(_sessions[id]));
        if(!sess)
            VTHROW(ServiceException, E_ARGS_INVALID, "Session " << id << " does not exist");
        return sess;
    }

    /**
     * @param cpu the cpu
     * @return the local thread for the given CPU to handle the provided portal
     */
    LocalThread *get_thread(cpu_t cpu) const {
        return _insts[cpu] != nullptr ? &_insts[cpu]->thread() : nullptr;
    }

protected:
    /**
     * Creates a new session in a free slot.
     *
     * @param cap a cap of the client that will be valid as long as the client exists
     * @return the created session
     * @throws ServiceException if there are no free slots anymore
     */
    ServiceSession *new_session(capsel_t cap);

private:
    /**
     * May be overwritten to create an inherited class from ServiceSession.
     *
     * @param id the session-id
     * @param cap a cap of the client that will be valid as long as the client exists
     * @param pts the capabilities
     * @param func the portal-function
     * @return the session-object
     */
    virtual ServiceSession *create_session(size_t id, capsel_t cap, capsel_t pts,
                                           Pt::portal_func func) {
        return new ServiceSession(this, id, cap, pts, func);
    }
    /**
     * Is called after a session has been created and put into the corresponding slot. May be
     * overwritten to perform some action.
     *
     * @param id the session-id
     */
    virtual void created_session(UNUSED size_t id) {
    }

    void reg() {
        UtcbFrame uf;
        uf.accept_delegates(0);
        uf.delegate(CapRange(_regcaps, 1 << CPU::order(), Crd::OBJ_ALL));
        uf << REGISTER << String(_name) << _reg_cpus;
        CPU::current().srv_pt().call(uf);
        uf.check_reply();
        _kill_sm = new Sm(uf.get_delegated(0).offset(), true);
    }
    void unreg() {
        UtcbFrame uf;
        uf << UNREGISTER << String(_name);
        CPU::current().srv_pt().call(uf);
        uf.check_reply();
    }

    void add_session(ServiceSession *sess) {
        rcu_assign_pointer(_sessions[sess->id()], sess);
        created_session(sess->id());
    }
    void remove_session(ServiceSession *sess) {
        rcu_assign_pointer(_sessions[sess->id()], nullptr);
        sess->invalidate();
        RCU::invalidate(sess);
        RCU::gc(true);
    }
    void check_sessions();
    void destroy_session(capsel_t pid);

    Service(const Service&);
    Service& operator=(const Service&);

    capsel_t _regcaps;
    capsel_t _caps;
    UserSm _sm;
    Sm *_kill_sm;
    bool _stop;
    const char *_name;
    Pt::portal_func _func;
    ServiceCPUHandler **_insts;
    BitField<Hip::MAX_CPUS> _reg_cpus;
    ServiceSession *_sessions[MAX_SESSIONS];
};

/**
 * The iterator to walk forwards or backwards over all sessions. We need that, because we have to
 * skip unused slots. Note that the iterator assumes that no sessions are destroyed while being
 * used. Sessions may be added or removed in the meanwhile.
 */
template<class T>
class SessionIterator {
    friend class Service;

public:
    /**
     * Creates an iterator that starts at given position
     *
     * @param s the service
     * @param pos the start-position (index into session-array)
     */
    explicit SessionIterator(Service *s, ssize_t pos = 0) : _s(s), _pos(pos), _last(next()) {
    }

    T & operator*() const {
        return *_last;
    }
    T *operator->() const {
        return &operator*();
    }
    SessionIterator & operator++() {
        if(_pos < static_cast<ssize_t>(Service::MAX_SESSIONS) - 1) {
            _pos++;
            _last = next();
        }
        return *this;
    }
    SessionIterator operator++(int) {
        SessionIterator<T> tmp(*this);
        operator++();
        return tmp;
    }
    SessionIterator & operator--() {
        if(_pos > 0) {
            _pos--;
            _last = prev();
        }
        return *this;
    }
    SessionIterator operator--(int) {
        SessionIterator<T> tmp(*this);
        operator++();
        return tmp;
    }
    bool operator==(const SessionIterator<T>& rhs) const {
        return _pos == rhs._pos;
    }
    bool operator!=(const SessionIterator<T>& rhs) const {
        return _pos != rhs._pos;
    }

private:
    T *next() {
        while(_pos < static_cast<ssize_t>(Service::MAX_SESSIONS)) {
            T *t = static_cast<T*>(rcu_dereference(_s->_sessions[_pos]));
            if(t)
                return t;
            _pos++;
        }
        return nullptr;
    }
    T *prev() {
        while(_pos >= 0) {
            T *t = static_cast<T*>(rcu_dereference(_s->_sessions[_pos]));
            if(t)
                return t;
            _pos--;
        }
        return nullptr;
    }

    Service* _s;
    ssize_t _pos;
    T *_last;
};

template<class T>
SessionIterator<T> Service::sessions_begin() {
    return SessionIterator<T>(this);
}

template<class T>
SessionIterator<T> Service::sessions_end() {
    return SessionIterator<T>(this, MAX_SESSIONS);
}

}
