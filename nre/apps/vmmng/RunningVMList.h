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
    static const size_t MAX_VMS     = 64;

    explicit RunningVMList() : _sm(), _max(), _list(is_less), _consoles() {
    }

public:
    static RunningVMList &get() {
        return _inst;
    }

    size_t count() const {
        return _list.length();
    }
    size_t max_idx() const {
        return _max;
    }
    void add(nre::ChildManager &cm, VMConfig *cfg, cpu_t cpu) {
        nre::ScopedLock<nre::UserSm> guard(&_sm);
        size_t console = alloc_console();
        try {
            nre::Child::id_type id = cfg->start(cm, console, cpu);
            nre::ScopedLock<nre::RCULock> rcuguard(&nre::RCU::lock());
            const nre::Child *child = cm.get(id);
            assert(child);
            _list.insert(new RunningVM(cfg, console, id, child->pd()));
            _max = nre::Math::max(_max, console);
        }
        catch(...) {
            free_console(console);
            throw;
        }
    }
    RunningVM *get(size_t idx) {
        nre::ScopedLock<nre::UserSm> guard(&_sm);
        auto it = _list.begin();
        for(; it != _list.end() && idx-- > 0; ++it)
            ;
        if(it == _list.end())
            return nullptr;
        return &*it;
    }
    RunningVM *get_by_pd(capsel_t pd) {
        nre::ScopedLock<nre::UserSm> guard(&_sm);
        for(auto it = _list.begin(); it != _list.end(); ++it) {
            if(it->pd() == pd)
                return &*it;
        }
        return nullptr;
    }
    void remove(RunningVM *vm) {
        nre::ScopedLock<nre::UserSm> guard(&_sm);
        if(_list.remove(vm)) {
            free_console(vm->console());
            delete vm;
        }
    }

private:
    size_t alloc_console() {
        // 0 is the management console
        for(size_t i = 1; i < MAX_VMS; ++i) {
            if(!_consoles[i]) {
                _consoles[i] = true;
                return i;
            }
        }
        throw nre::Exception(nre::E_CAPACITY, "All VM consoles in use");
    }
    void free_console(size_t i) {
        _consoles[i] = false;
        for(size_t i = 1; i < MAX_VMS; ++i) {
            if(_consoles[i])
                _max = i;
        }
    }

    static bool is_less(const RunningVM &a, const RunningVM &b) {
        return a.console() < b.console();
    }

    nre::UserSm _sm;
    size_t _max;
    nre::SortedSList<RunningVM> _list;
    bool _consoles[MAX_VMS];
    static RunningVMList _inst;
};
