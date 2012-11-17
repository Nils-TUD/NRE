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

#include <mem/RegionManager.h>
#include <stream/OStream.h>

namespace nre {

OStream &operator<<(OStream &os, const RegionManager &rm) {
    for(size_t i = 0; i < RegionManager::MAX_REGIONS; ++i) {
        const RegionManager::Region *r = rm._regs + i;
        if(r->size > 0) {
            os << "\t" << i << ": " << fmt(r->addr, "p") << " .. " << fmt(r->addr + r->size, "p")
               << " (" << r->size << ")\n";
        }
    }
    return os;
}

}
