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
 * Represents a bus-device-function triple. The main purpose of this class is to provide an OStream-
 * operator for easy printing. Additionally, extracting the fields from a BDF is simplified by
 * this class.
 */
class BDF {
public:
    typedef uint32_t bdf_type;

    /**
     * Empty constructor
     */
    explicit BDF() : _bdf() {
    }
    /**
     * Takes the given bdf as it is
     */
    explicit BDF(bdf_type bdf) : _bdf(bdf) {
    }
    /**
     * Builds a bdf-triple from <bus>, <dev> and <func>. Assumes that they are disjunct.
     */
    explicit BDF(bdf_type bus, bdf_type dev, bdf_type func) : _bdf((bus << 8) | (dev << 3) | func) {
    }

    /**
     * @return the complete triple
     */
    bdf_type value() const {
        return _bdf;
    }
    /**
     * @return the bus
     */
    bdf_type bus() const {
        return (_bdf >> 8) & 0xFF;
    }
    /**
     * @return the device
     */
    bdf_type device() const {
        return (_bdf >> 3) & 0x1F;
    }
    /**
     * @return the function
     */
    bdf_type function() const {
        return _bdf & 0x7;
    }

private:
    bdf_type _bdf;
};

/**
 * Comparison operators
 */
static inline bool operator==(const BDF &bdf1, const BDF &bdf2) {
    return bdf1.value() == bdf2.value();
}
static inline bool operator!=(const BDF &bdf1, const BDF &bdf2) {
    return !operator==(bdf1, bdf2);
}

/**
 * OStream operator
 */
static inline OStream &operator<<(OStream &os, const BDF &bdf) {
    os.writef("BDF[%02x:%02x.%x]", bdf.bus(), bdf.device(), bdf.function());
    return os;
}

}
