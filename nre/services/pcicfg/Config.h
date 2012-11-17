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
#include <services/PCIConfig.h>

class Config {
public:
    typedef nre::PCIConfig::value_type value_type;

    explicit Config() {
    }
    virtual ~Config() {
    }

    virtual const char *name() const = 0;
    virtual bool contains(nre::BDF bdf, size_t offset) const = 0;
    virtual uintptr_t addr(nre::BDF bdf, size_t offset) = 0;
    virtual value_type read(nre::BDF bdf, size_t offset) = 0;
    virtual void write(nre::BDF bdf, size_t offset, value_type value) = 0;
};
