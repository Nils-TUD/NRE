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

namespace nre {

/**
 * Some routines for floating point numbers
 */
template<typename T>
class Float {
    /**
     * @return the value that represents NaN
     */
    static T nan();
    /**
     * @return the value that represents infinity
     */
    static T inf();
    /**
     * @return true if <v> is negative, which takes care of NaN
     */
    static bool is_negative(T v);
    /**
     * @return true if <v> is NaN
     */
    static bool is_nan(T v);
    /**
     * @return true if <v> is +/i infinity
     */
    static bool is_inf(T v);
};

class FloatBase {
protected:
    template<typename Flt, typename Int>
    union FloatInt {
        Flt f;
        Int i;
        explicit FloatInt(Flt f) : f(f) {
        }
        explicit FloatInt(Int i) : i(i) {
        }
    };
};

template<>
class Float<float> : public FloatBase {
    typedef FloatInt<float, uint32_t> union_type;
public:
    static float nan() {
        union_type val(static_cast<uint32_t>(0x7FC00000));
        return val.f;
    }
    static float inf() {
        union_type val(static_cast<uint32_t>(0x7F800000));
        return val.f;
    }
    static bool is_negative(float v) {
        union_type val(v);
        return val.i & 0x80000000;
    }
    static bool is_nan(float v) {
        union_type val(v);
        return ((val.i >> 23) & 0xFF) == 0xFF && (val.i & 0x7FFFFF) != 0;
    }
    static bool is_inf(float v) {
        union_type val(v);
        return (val.i & 0x7FFFFFFF) == 0x7FF00000;
    }
};

template<>
class Float<double> : public FloatBase {
    typedef FloatInt<double, uint64_t> union_type;
public:
    static double nan() {
        union_type val(static_cast<uint64_t>(0x7FF8000000000000));
        return val.f;
    }
    static double inf() {
        union_type val(static_cast<uint64_t>(0x7FF0000000000000));
        return val.f;
    }
    static bool is_negative(double v) {
        union_type val(v);
        return val.i & 0x8000000000000000ULL;
    }
    static bool is_nan(double v) {
        union_type val(v);
        return ((val.i >> 52) & 0x7FF) == 0x7FF && (val.i & 0xFFFFFFFFFFFFFULL) != 0;
    }
    static int is_inf(double v) {
        union_type val(v);
        return (val.i & 0x7FFFFFFFFFFFFFFFULL) == 0x7FF0000000000000ULL;
    }
};

}
