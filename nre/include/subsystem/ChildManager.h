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

#include <kobj/Pt.h>
#include <kobj/LocalThread.h>
#include <ipc/Service.h>
#include <subsystem/ServiceRegistry.h>
#include <subsystem/ChildConfig.h>
#include <mem/DataSpaceManager.h>
#include <util/Sync.h>
#include <Exception.h>

namespace nre {

/**
 * The exception used for errors during loading of the ELF file
 */
class ElfException : public Exception {
public:
    DEFINE_EXCONSTRS(ElfException)
};

/**
 * For all other exceptions regarding child handling
 */
class ChildException : public Exception {
public:
    DEFINE_EXCONSTRS(ChildException)
};

/**
 * This class is responsible for managing child tasks. That is, it provides portals for the child
 * tasks for various purposes (dataspaces, I/O ports, GSIs, services, ...). It will also handle
 * exceptions like pagefaults, division by zero and so on and react appropriately. It allows you
 * to load child tasks and get information about the running ones.
 */
class ChildManager {
    class Portals;
    friend class Portals;
    friend class Child;

    /**
     * Holds all portals for the child tasks
     */
    class Portals {
    public:
        static const size_t COUNT   = 9;

        PORTAL static void startup(capsel_t pid);
        PORTAL static void init_caps(capsel_t pid);
        PORTAL static void service(capsel_t pid);
        PORTAL static void io(capsel_t pid);
        PORTAL static void sc(capsel_t pid);
        PORTAL static void gsi(capsel_t pid);
        PORTAL static void dataspace(capsel_t pid);
        PORTAL static void pf(capsel_t pid);
        PORTAL static void exception(capsel_t pid);
    };

    /**
     * The different exit types
     */
    enum ExitType {
        THREAD_EXIT,
        PROC_EXIT,
        FAULT
    };

public:
    /**
     * Some settings
     */
    static const size_t MAX_CHILDS          = 32;
    static const size_t MAX_CMDLINE_LEN     = 256;
    static const size_t MAX_MODAUX_LEN      = ExecEnv::PAGE_SIZE;

    /**
     * Creates a new child manager. It will already create all Ecs that are required
     */
    explicit ChildManager();
    /**
     * Deletes this child manager, i.e. it kills and deletes all childs and deletes all Ecs
     */
    ~ChildManager();

    /**
     * Loads a child task. That is, it treats <addr>...<addr>+<size> as an ELF file, creates a new
     * Pd, adds the correspondings segments to that Pd, creates a main thread and finally starts
     * the main thread. Afterwards, if the command line contains "provides=..." it waits until
     * the service with given name is registered.
     *
     * @param addr the address of the ELF file
     * @param size the size of the ELF file
     * @param config the config to use. this allows you to specify the access to the modules, the
     *  presented CPUs and other things
     * @return the id of the created child
     * @throws ELFException if the ELF is invalid
     * @throws Exception if something else failed
     */
    Child::id_type load(uintptr_t addr, size_t size, const ChildConfig &config);

    /**
     * @return the number of childs
     */
    size_t count() const {
        return _child_count;
    }
    /**
     * @return a semaphore that is up'ed as soon as a child has been killed
     */
    Sm &dead_sm() {
        return _diesm;
    }

    /**
     * Returns the child with given id. Note that the childs are managed by RCU. So, you should
     * use a RCULock to mark the read access and just use this method once at the beginning and
     * store it in a variable. If its none-zero, you can use it without trouble.
     *
     * @param id the child id
     * @return the child with given id or 0 if not existing
     */
    const Child *get(Child::id_type id) const {
        return get_at((id - _portal_caps) / per_child_caps());
    }
    /**
     * Returns the child at given index. Note that the childs are managed by RCU. So, you should
     * use a RCULock to mark the read access and just use this method once at the beginning and
     * store it in a variable. If its none-zero, you can use it without trouble.
     *
     * @param idx the index of the child
     * @return the child at given index or 0 if not existing
     */
    const Child *get_at(size_t idx) const {
        return rcu_dereference(_childs[idx]);
    }

    /**
     * Kills the child with given id
     *
     * @param id the child id
     */
    void kill(Child::id_type id) {
        destroy_child(id);
    }

    /**
     * @return the service registry
     */
    const ServiceRegistry &registry() const {
        return _registry;
    }
    /**
     * Registers the given service. This is used to let the task that hosts the childmanager
     * register services as well (by default, only its child tasks do so).
     *
     * @param cap the portal capabilities
     * @param name the service name
     * @param available the CPUs it is available on
     * @return a semaphore cap that is used to notify the service about potentially destroyed sessions
     */
    capsel_t reg_service(capsel_t cap, const String& name, const BitField<Hip::MAX_CPUS> &available) {
        return reg_service(0, cap, name, available);
    }
    /**
     * Unregisters the service with given name
     *
     * @param name the service name
     */
    void unreg_service(const String& name) {
        unreg_service(0, name);
    }

private:
    size_t free_slot() const {
        ScopedLock<UserSm> guard(&_slotsm);
        for(size_t i = 0; i < MAX_CHILDS; ++i) {
            if(_childs[i] == 0)
                return i;
        }
        throw ChildException(E_CAPACITY, "No free child slots");
    }

    Child *get_child(Child::id_type id) {
        return get_child_at((id - _portal_caps) / per_child_caps());
    }
    Child *get_child_at(size_t idx) {
        Child *c = rcu_dereference(_childs[idx]);
        if(!c)
            throw ChildException(E_NOT_FOUND, 32, "Child with idx %zu does not exist", idx);
        return c;
    }
    static inline size_t per_child_caps() {
        return Math::next_pow2(Hip::get().service_caps() * CPU::count());
    }
    cpu_t get_cpu(capsel_t pid) const {
        size_t off = (pid - _portal_caps) % per_child_caps();
        return off / Hip::get().service_caps();
    }
    uint get_vector(capsel_t pid) const {
        return (pid - _portal_caps) % Hip::get().service_caps();
    }

    const ServiceRegistry::Service *get_service(const String &name) {
        ScopedLock<UserSm> guard(&_sm);
        const ServiceRegistry::Service* s = registry().find(name);
        if(!s) {
            if(!_startup_info.child)
                throw ChildException(E_NOT_FOUND, 64, "Unable to find service '%s'", name.str());
            BitField<Hip::MAX_CPUS> available;
            capsel_t pts = get_parent_service(name.str(), available);
            s = _registry.reg(0, name, pts, 1 << CPU::order(), available);
            _regsm.up();
        }
        return s;
    }
    capsel_t reg_service(Child *c, capsel_t pts, const String& name,
                         const BitField<Hip::MAX_CPUS> &available) {
        ScopedLock<UserSm> guard(&_sm);
        const ServiceRegistry::Service *srv = _registry.reg(c, name, pts, 1 << CPU::order(), available);
        _regsm.up();
        return srv->sm().sel();
    }
    void unreg_service(Child *c, const String& name) {
        ScopedLock<UserSm> guard(&_sm);
        _registry.unreg(c, name);
    }
    void notify_services() {
        {
            ScopedLock<UserSm> guard(&_sm);
            for(ServiceRegistry::iterator it = _registry.begin(); it != _registry.end(); ++it)
                it->sm().up();
        }
        UtcbFrame uf;
        uf << Service::CLIENT_DIED;
        CPU::current().srv_pt().call(uf);
    }

    void term_child(capsel_t pid, UtcbExcFrameRef &uf);
    void kill_child(capsel_t pid, UtcbExcFrameRef &uf, ExitType type);
    void destroy_child(capsel_t pid);

    static void prepare_stack(Child *c, uintptr_t &sp, uintptr_t csp);
    void build_hip(Child *c, const ChildConfig &config);

    capsel_t get_parent_service(const char *name, BitField<Hip::MAX_CPUS> &available);
    void map(UtcbFrameRef &uf, Child *c, DataSpace::RequestType type);
    void switch_to(UtcbFrameRef &uf, Child *c);
    void unmap(UtcbFrameRef &uf, Child *c);

    ChildManager(const ChildManager&);
    ChildManager& operator=(const ChildManager&);

    size_t _child_count;
    Child *_childs[MAX_CHILDS];
    capsel_t _portal_caps;
    DataSpaceManager<DataSpace> _dsm;
    ServiceRegistry _registry;
    UserSm _sm;
    UserSm _switchsm;
    mutable UserSm _slotsm;
    Sm _regsm;
    Sm _diesm;
    // we need different Ecs to be able to receive a different number of caps
    LocalThread **_ecs;
    LocalThread **_regecs;
};

}
