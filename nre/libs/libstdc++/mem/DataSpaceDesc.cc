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

#include <mem/DataSpaceDesc.h>
#include <stream/OStream.h>

namespace nre {

OStream &operator<<(OStream &os, const DataSpaceDesc &desc) {
    os << "virt=" << fmt(desc.virt(), "p") << " phys=" << fmt(desc.phys(), "p")
       << " size=" << desc.size() << " org=" << fmt(desc.origin(), "p")
       << " flags=" << fmt(desc.flags(), "#x") << " align=" << desc.align();
    return os;
}

}
