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

#include <subsystem/Child.h>
#include <ipc/Producer.h>
#include <services/VMManager.h>
#include <util/SList.h>

#include "VMConfig.h"

class RunningVM : public nre::SListItem {
public:
    explicit RunningVM(VMConfig *cfg, size_t console, nre::Child::id_type id, capsel_t pd)
        : nre::SListItem(), _cfg(cfg), _console(console), _id(id), _pd(pd), _prod() {
    }

    const VMConfig *cfg() const {
        return _cfg;
    }
    size_t console() const {
        return _console;
    }
    nre::Child::id_type id() const {
        return _id;
    }
    capsel_t pd() const {
        return _pd;
    }

    bool initialized() const {
        return _prod != nullptr;
    }
    void set_producer(nre::Producer<nre::VMManager::Packet> *prod) {
        _prod = prod;
    }
    void execute(nre::VMManager::Command cmd) {
        assert(_prod);
        nre::VMManager::Packet pk;
        pk.cmd = cmd;
        _prod->produce(pk);
    }

private:
    VMConfig *_cfg;
    size_t _console;
    nre::Child::id_type _id;
    capsel_t _pd;
    nre::Producer<nre::VMManager::Packet> *_prod;
};
