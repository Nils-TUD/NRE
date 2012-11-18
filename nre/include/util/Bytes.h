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
 * Convenience class to print a size with the corresponding binary prefix, depending on how large
 * it is. To do that you can write an instance of it into an OStream.
 */
class Bytes {
public:
    static const size_t KIB = 1024;
    static const size_t MIB = 1024 * KIB;
    static const size_t GIB = 1024 * MIB;

    explicit Bytes(size_t size) : _size(size) {
    }

    size_t size() const {
        return _size;
    }

private:
    size_t _size;
};

static inline OStream &operator<<(OStream &os, const Bytes &bp) {
    if(bp.size() >= Bytes::GIB)
        os << fmt(static_cast<double>(bp.size()) / Bytes::GIB, 0U, 1) << " GiB";
    else if(bp.size() >= Bytes::MIB)
        os << fmt(static_cast<double>(bp.size()) / Bytes::MIB, 0U, 1) << " MiB";
    else if(bp.size() >= Bytes::KIB)
        os << fmt(static_cast<double>(bp.size()) / Bytes::KIB, 0U, 1) << " KiB";
    else
        os << bp.size() << " Bytes";
    return os;
}

}
