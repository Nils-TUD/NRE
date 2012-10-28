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
#include <RCU.h>
#include <CPU.h>

namespace nre {

class Service;

/**
 * The server-part of a session. This way the service can manage per-session-data. That is,
 * it can distinguish between clients. Note that sessions are tracked by RCU. So, whenever you
 * access a session (by Service::get_session, for example), make sure that you use a RCULock.
 */
class ServiceSession : public RCUObject {
    friend class Service;
    friend class ServiceCPUHandler;

public:
    /**
     * Constructor
     *
     * @param s the service-instance
     * @param id the id of this session
     * @param cap a cap of the client that will be valid as long as the client exists
     * @param pts the portal selectors
     * @param func the portal function
     *  Otherwise, portals are created
     */
    explicit ServiceSession(Service *s, size_t id, capsel_t cap, capsel_t pts, Pt::portal_func func);
    /**
     * Destroyes this session
     */
    virtual ~ServiceSession() {
        for(uint i = 0; i < CPU::count(); ++i)
            delete _pts[i];
        delete[] _pts;
    }

    /**
     * @return the session-id
     */
    size_t id() const {
        return _id;
    }
    /**
     * @return a cap of the client that will be valid as long as the client exists
     */
    capsel_t cap() const {
        return _cap;
    }
    /**
     * @return the capabilities
     */
    capsel_t portal_caps() const {
        return _caps;
    }

protected:
    /**
     * Is called if this session should be destroyed. May be overwritten to give the session a
     * chance to do some cleanup or similar.
     */
    virtual void invalidate() {
    }

private:
    size_t _id;
    capsel_t _cap;
    capsel_t _caps;
    Pt **_pts;
};

}
