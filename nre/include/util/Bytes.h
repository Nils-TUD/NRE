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

#include <stream/OStream.h>

namespace nre {

/**
 * Convenience class to print a number of bytes with the corresponding binary prefix, depending on
 * the bytecount. To do that you can write an instance of it into an OStream.
 */
class Bytes {
public:
    static const size_t KIB = 1024;
    static const size_t MIB = 1024 * KIB;
    static const size_t GIB = 1024 * MIB;

    explicit Bytes(size_t count) : _count(count) {
    }

    size_t count() const {
        return _count;
    }

private:
    size_t _count;
};

static inline OStream &operator<<(OStream &os, const Bytes &bp) {
    if(bp.count() >= Bytes::GIB)
        os << fmt(static_cast<double>(bp.count()) / Bytes::GIB, 0U, 1) << " GiB";
    else if(bp.count() >= Bytes::MIB)
        os << fmt(static_cast<double>(bp.count()) / Bytes::MIB, 0U, 1) << " MiB";
    else if(bp.count() >= Bytes::KIB)
        os << fmt(static_cast<double>(bp.count()) / Bytes::KIB, 0U, 1) << " KiB";
    else
        os << bp.count() << " Bytes";
    return os;
}

}
