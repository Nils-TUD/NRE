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

#include <arch/Startup.h>
#include <RCU.h>

namespace nre {

class Init {
    Init() {
        RCU::add(Thread::current());
    }
    static Init init;
};

uint32_t *RCU::_versions = 0;
size_t RCU::_versions_count = 0;
SList<Thread> RCU::_ecs;
RCUObject *RCU::_objs = 0;
RCULock RCU::_lock;
UserSm RCU::_sm INIT_PRIO_RCU;
UserSm RCU::_ecsm INIT_PRIO_RCU;
Init Init::init INIT_PRIO_RCU;

}
