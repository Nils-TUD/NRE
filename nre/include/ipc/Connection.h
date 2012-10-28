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

#include <utcb/UtcbFrame.h>
#include <ipc/Service.h>
#include <cap/CapSelSpace.h>
#include <kobj/Pt.h>
#include <util/BitField.h>
#include <CPU.h>

namespace nre {

/**
 * Represents a connection to a service in the sense that we have portals to open and close sessions
 * at the service.
 */
class Connection {
public:
    /**
     * Opens the connection to the service with given name
     *
     * @param service the service-name
     * @throws Exception if the connection failed
     */
    explicit Connection(const char *service)
        : _available(), _caps(connect(service)), _pts(new Pt *[CPU::count()]) {
        for(size_t i = 0; i < CPU::count(); ++i)
            _pts[i] = 0;
    }
    /**
     * Closes the connection
     */
    ~Connection() {
        for(size_t i = 0; i < CPU::count(); ++i)
            delete _pts[i];
        delete[] _pts;
        CapSelSpace::get().free(_caps, 1 << CPU::order());
    }

    /**
     * @param log_id the logical CPU id
     * @return true if you can use the given CPU to talk to the service
     */
    bool available_on(cpu_t log_id) const {
        return _available.is_set(log_id);
    }
    /**
     * @param log_id the logical CPU id
     * @return the portal for CPU <cpu>
     */
    Pt *pt(cpu_t log_id) {
        assert(available_on(log_id));
        if(_pts[log_id] == 0)
            _pts[log_id] = new Pt(_caps + log_id);
        return _pts[log_id];
    }

private:
    capsel_t connect(const char *service) {
        UtcbFrame uf;
        ScopedCapSels caps(1 << CPU::order(), 1 << CPU::order());
        uf.delegation_window(Crd(caps.get(), CPU::order(), Crd::OBJ_ALL));
        uf << Service::GET << String(service);
        CPU::current().srv_pt().call(uf);
        uf.check_reply();
        uf >> _available;
        return caps.release();
    }

    Connection(const Connection&);
    Connection& operator=(const Connection&);

    BitField<Hip::MAX_CPUS> _available;
    capsel_t _caps;
    Pt **_pts;
};

}
