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

#include <util/BitField.h>
#include <Hip.h>

namespace nre {

/**
 * This class is used to choose CPUs. Currently this is used by the service to let the user
 * specify on what CPUs it should be available. This class is just a wrapper for BitField that
 * exists for convenience.
 */
class CPUSet {
public:
    /**
     * The initial settings
     */
    enum Preset {
        NONE,
        ALL
    };

    /**
     * Constructor. Preinitializes the set to <preset>
     *
     * @param preset the CPUs to enable at the beginning (ALL by default)
     */
    explicit CPUSet(Preset preset = ALL) : _cpus() {
        if(preset == ALL)
            _cpus.set_all();
    }

    /**
     * @return the CPUs in form of a bitfield
     */
    const BitField<Hip::MAX_CPUS> &get() const {
        return _cpus;
    }
    /**
     * Sets the given CPU
     *
     * @param cpu the CPU
     */
    void set(cpu_t cpu) {
        _cpus.set(cpu);
    }

private:
    BitField<Hip::MAX_CPUS> _cpus;
};

}
