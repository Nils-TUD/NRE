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

#include <cap/CapRange.h>
#include <stream/OStream.h>
#include <util/Math.h>
#include <Desc.h>
#include <Syscalls.h>

namespace nre {

void CapRange::revoke(bool self) {
    uintptr_t start = _start;
    size_t count = _count;
    while(count > 0) {
        uint minshift = Math::minshift(start, count);
        Syscalls::revoke(Crd(start, minshift, _attr), self);
        start += 1 << minshift;
        count -= 1 << minshift;
    }
}

void CapRange::limit_to(size_t free_typed) {
    uintptr_t hs = hotspot() != CapRange::NO_HOTSPOT ? hotspot() : start();
    size_t c = count();
    uintptr_t st = start();
    while(free_typed > 0 && c > 0) {
        uint minshift = Math::minshift(st | hs, c);
        st += 1 << minshift;
        hs += 1 << minshift;
        c -= 1 << minshift;
        free_typed--;
    }
    _count = count() - c;
}

OStream &operator<<(OStream &os, const CapRange &cr) {
    os << "CapRange[start=" << fmt(cr.start(), "#x") << ", count=" << fmt(cr.count(), "#x")
       << ", hotspot=" << fmt(cr.hotspot(), "#x") << ", attr=" << fmt(cr.attr(), "#x") << "]";
    return os;
}

}
