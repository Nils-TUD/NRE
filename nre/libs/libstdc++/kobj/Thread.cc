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

#include <kobj/Thread.h>
#include <kobj/LocalThread.h>
#include <kobj/Sc.h>
#include <kobj/Pt.h>
#include <utcb/UtcbFrame.h>
#include <Compiler.h>
#include <CPU.h>
#include <RCU.h>

namespace nre {

// slot 0 is reserved
size_t Thread::_tls_idx = 1;

Thread::Thread(Pd *pd, Syscalls::ECType type, ExecEnv::startup_func start, uintptr_t ret, cpu_t cpu,
               capsel_t evb, uintptr_t stack, uintptr_t uaddr)
    : Ec(cpu, evb, create(this, pd, type, cpu, evb, start, ret, uaddr, stack, _flags)),
      SListItem(), _rcu_counter(0), _utcb_addr(uaddr), _stack_addr(stack), _tls() {
}

Thread::Thread(cpu_t cpu, capsel_t evb, capsel_t cap, uintptr_t stack, uintptr_t uaddr)
    : Ec(cpu, evb, cap), SListItem(), _rcu_counter(0), _utcb_addr(uaddr), _stack_addr(stack),
      _flags(), _tls() {
}

capsel_t Thread::create(Thread *t, Pd *pd, Syscalls::ECType type, cpu_t cpu, capsel_t evb,
                        ExecEnv::startup_func start, uintptr_t ret, uintptr_t &uaddr,
                        uintptr_t &stack, uint &flags) {
    // request stack and utcb from parent, if necessary
    flags = HAS_OWN_STACK | HAS_OWN_UTCB;
    if(stack == 0 || uaddr == 0) {
        UtcbFrame uf;
        uf << Sc::CREATE << (stack == 0) << (uaddr == 0);
        CPU::current().sc_pt().call(uf);
        uf.check_reply();
        if(stack == 0) {
            uf >> stack;
            flags &= ~HAS_OWN_STACK;
        }
        if(uaddr == 0) {
            uf >> uaddr;
           flags &= ~HAS_OWN_UTCB;
        }
    }

    // setup stack and create Ec
    void *sp = ExecEnv::setup_stack(pd, t, start, ret, stack);
    ScopedCapSels scs;
    Syscalls::create_ec(scs.get(), reinterpret_cast<void*>(uaddr), sp, CPU::get(cpu).phys_id(),
                        evb, type, pd->sel());
    if(pd == Pd::current())
        RCU::add(t);
    return scs.release();
}

Thread::~Thread() {
    RCU::remove(this);
}

}
