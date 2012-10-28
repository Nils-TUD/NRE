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

namespace nre {

/**
 * Synchronization primitives.
 */
class Sync {
public:
    /**
     * Prevents the compiler from reordering instructions. That is, the code-generator will put all
     * preceding load and store commands before load and store commands that follow this call.
     */
    static inline void memory_barrier() {
        asm volatile ("" : : : "memory");
    }

    /**
     * Ensures that all loads and stores before this call are globally visible before all loads and
     * stores after this call.
     */
    static inline void memory_fence() {
        asm volatile ("mfence" : : : "memory");
    }
    /**
     * Ensures that all loads before this call are globally visible before all loads after this call.
     */
    static inline void load_fence() {
        asm volatile ("lfence" : : : "memory");
    }
    /**
     * Ensures that all stores before this call are globally visible before all stores after this call.
     */
    static inline void store_fence() {
        asm volatile ("sfence" : : : "memory");
    }

private:
    Sync();
};

}
