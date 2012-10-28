/** @file
 * Halifax - an instruction emulator.
 *
 * Copyright (C) 2009-2010, Bernhard Kauer <bk@vmmon.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of Vancouver.
 *
 * Vancouver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Vancouver is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#include <arch/Defines.h>
#include "../bus/motherboard.h"
#include "../bus/vcpu.h"
#include "cpustate.h"
#include "instcache.h"

using namespace nre;

/**
 * Halifax: an instruction emulator.
 */
class Halifax : public InstructionCache, public StaticReceiver<Halifax> {
public:
    bool receive(CpuMessage &msg) {
        if(msg.type != CpuMessage::TYPE_SINGLE_STEP)
            return false;
        step(msg);
        return true;
    }

    Halifax(VCVCpu *vcpu) : InstructionCache(vcpu) {
        vcpu->executor.add(this, receive_static);
    }
    void *operator new(size_t size) {
        return new /* TODO(__alignof__(Halifax))  */ char[size];
    }
};

PARAM_HANDLER(halifax,
              "halifax - create a halifax that emulatates instructions.") {
    if(!mb.last_vcpu)
        throw Exception(E_NOT_FOUND, "no VCPU for this Halifax");
    new Halifax(mb.last_vcpu);
}
