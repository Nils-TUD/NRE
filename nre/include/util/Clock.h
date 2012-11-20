/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2007-2008, Bernhard Kauer <bk@vmmon.org>
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

#include <util/Math.h>
#include <util/Util.h>
#include <Hip.h>

namespace nre {

/**
 * A clock can be used to convert time from TSC to a different time domain and vice versa.
 */
class Clock {
public:
    /**
     * Constructor
     *
     * @param dst_freq the destination frequency to/from which you want to convert
     */
    explicit Clock(timevalue_t dst_freq) : _src_freq(Hip::get().freq_tsc * 1000), _dst_freq(dst_freq) {
    }

    /**
     * @return the source frequency (i.e. the frequency of the TSC)
     */
    timevalue_t source_freq() const {
        return _src_freq;
    }
    /**
     * @return the destination frequency
     */
    timevalue_t dest_freq() const {
        return _dst_freq;
    }

    /**
     * @return current TSC value
     */
    timevalue_t source_time() const {
        return Util::tsc();
    }
    /**
     * @param delta the delta to add to the current TSC value in the destination frequency
     * @return the TSC value of now + the given delta. That is, if you pass delta=5 and have a
     *  destination frequency of 1000, it will return <now> + 5ms as TSC value.
     */
    timevalue_t source_time(timevalue_t delta) const {
        return source_time(delta, _dst_freq);
    }
    /**
     * @param delta the delta to add to the current TSC value in <freq>
     * @param freq the frequency of delta
     * @return the TSC value of now + the given delta. That is, if you pass delta=5 and freq=1000,
     *  it will return <now> + 5ms as TSC value.
     */
    timevalue_t source_time(timevalue_t delta, timevalue_t freq) const {
        return source_time() + Math::muldiv128(delta, _src_freq, freq);
    }

    /**
     * @return the current TSC value into the destination frequency
     */
    timevalue_t dest_time() const {
        return dest_time_of(source_time());
    }
    /**
     * @param tsc the TSC value
     * @return the given TSC value into the destination frequency
     */
    timevalue_t dest_time_of(timevalue_t tsc) const {
        return Math::muldiv128(tsc, _dst_freq, _src_freq);
    }
    /**
     * Computes the delta in the destination frequency from now to the given TSC value.
     *
     * @param tsc the TSC value
     * @return the delta in the destination frequency
     */
    timevalue_t dest_delta(timevalue_t tsc) const {
        return delta(_dst_freq, tsc);
    }

    /**
     * @param freq the frequency
     * @return the current time in the given frequency
     */
    timevalue_t time(timevalue_t freq) const {
        return time_of(freq, source_time());
    }
    /**
     * @param freq the frequency
     * @param tsc the time
     * @return the given time in the given frequency
     */
    timevalue_t time_of(timevalue_t freq, timevalue_t tsc) const {
        return Math::muldiv128(tsc, freq, _src_freq);
    }
    /**
     * @param freq the frequency
     * @param tsc the time
     * @return the delta of now and the given time in the given frequency
     */
    timevalue_t delta(timevalue_t freq, timevalue_t tsc) const {
        timevalue_t now = source_time();
        if(now > tsc)
            return 0;
        return Math::muldiv128(tsc - now, freq, _src_freq);
    }

private:
    timevalue_t _src_freq;
    timevalue_t _dst_freq;
};

}
