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

#include <ipc/Service.h>
#include <ipc/ServiceSession.h>

namespace nre {

ServiceSession::ServiceSession(Service *s, size_t id, capsel_t cap, capsel_t pts, Pt::portal_func func)
    : RCUObject(), _id(id), _cap(cap), _caps(pts), _pts(new Pt *[CPU::count()]) {
    for(uint i = 0; i < CPU::count(); ++i) {
        _pts[i] = 0;
        if(s->available().is_set(i)) {
            LocalThread *ec = s->get_thread(i);
            assert(ec != 0);
            _pts[i] = new Pt(ec, pts + i, func);
        }
    }
}

}
