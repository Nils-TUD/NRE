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

#include <arch/Types.h>
#include <arch/Defines.h>
#include <util/Atomic.h>
#include <util/Util.h>

typedef word_t spinlock_t;

static inline void lock(spinlock_t *val) {
    while(!nre::Atomic::cmpnswap(val, 0, 1))
        nre::Util::pause();
}

static inline void unlock(spinlock_t *val) {
    // Instead of *val = 0, we use the builtin function, because it
    // also generates a release memory barrier.
    __sync_lock_release(val);
}

#ifdef __cplusplus
namespace nre {

class SpinLock {
public:
    SpinLock() : _val() {
    }

    void down() {
        lock(&_val);
    }
    void up() {
        unlock(&_val);
    }

private:
    SpinLock(const SpinLock&);
    SpinLock& operator=(const SpinLock&);

    spinlock_t _val;
};

}
#endif
