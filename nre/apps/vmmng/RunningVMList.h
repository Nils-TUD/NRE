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

#include <kobj/UserSm.h>
#include <util/ScopedLock.h>
#include <util/SList.h>

#include "RunningVM.h"

class RunningVMList {
    explicit RunningVMList() : _sm(), _list() {
    }

public:
    static RunningVMList &get() {
        return _inst;
    }

    size_t count() const {
        return _list.length();
    }
    void add(nre::ChildManager &cm, VMConfig *cfg, cpu_t cpu) {
        nre::ScopedLock<nre::UserSm> guard(&_sm);
        size_t console = count() + 1;
        nre::Child::id_type id = cfg->start(cm, console, cpu);
        nre::ScopedLock<nre::RCULock> rcuguard(&nre::RCU::lock());
        const nre::Child *child = cm.get(id);
        if(child)
            _list.append(new RunningVM(cfg, id, child->pd()));
    }
    RunningVM *get(size_t idx) {
        nre::ScopedLock<nre::UserSm> guard(&_sm);
        nre::SList<RunningVM>::iterator it;
        for(it = _list.begin(); it != _list.end() && idx-- > 0; ++it)
            ;
        if(it == _list.end())
            return nullptr;
        return &*it;
    }
    RunningVM *get_by_pd(capsel_t pd) {
        nre::ScopedLock<nre::UserSm> guard(&_sm);
        for(nre::SList<RunningVM>::iterator it = _list.begin(); it != _list.end(); ++it) {
            if(it->pd() == pd)
                return &*it;
        }
        return nullptr;
    }
    void remove(RunningVM *vm) {
        nre::ScopedLock<nre::UserSm> guard(&_sm);
        if(_list.remove(vm))
            delete vm;
    }

private:
    nre::UserSm _sm;
    nre::SList<RunningVM> _list;
    static RunningVMList _inst;
};
