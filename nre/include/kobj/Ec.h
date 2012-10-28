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

#include <kobj/ObjCap.h>
#include <Syscalls.h>
#include <Hip.h>

namespace nre {

class Pd;

/**
 * Represents an execution context. This class can't be instantiated directly. To create an Ec, use
 * LocalThread, GlobalThread or VCPU.
 */
class Ec : public ObjCap {
protected:
    /**
     * Constructor
     *
     * @param cpu the logical cpu to bind the Ec to
     * @param evb the offset for the event-portals
     * @param cap the capability (INVALID if a new one should be used)
     */
    explicit Ec(cpu_t cpu, capsel_t evb, capsel_t cap = INVALID)
        : ObjCap(cap), _event_base(evb), _cpu(cpu) {
    }

public:
    /**
     * Attaches an Ec-object to the given selector. The destructor will neither free the selector
     * nor the capability.
     *
     * @param cap the capability-selector for the Ec
     * @param cpu the logical CPU it runs on
     */
    explicit Ec(capsel_t cap, cpu_t cpu)
        : ObjCap(cap, KEEP_CAP_BIT | KEEP_SEL_BIT), _event_base(Hip::get().service_caps() * cpu),
          _cpu(cpu) {
    }

    /**
     * Let's this Ec perform a recall
     */
    void recall() {
        Syscalls::ec_ctrl(sel(), Syscalls::RECALL);
    }

    /**
     * @return the logical cpu id
     */
    cpu_t cpu() const {
        return _cpu;
    }
    /**
     * @return the event-base
     */
    capsel_t event_base() const {
        return _event_base;
    }

private:
    Ec(const Ec&);
    Ec& operator=(const Ec&);

    capsel_t _event_base;
    cpu_t _cpu;
};

}
