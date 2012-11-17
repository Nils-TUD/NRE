/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2009-2010, Bernhard Kauer <bk@vmmon.org>
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

#include "HostPCIConfig.h"

using namespace nre;

BDF HostPCIConfig::search_device(value_type theclass, value_type subclass,
                                                     uint inst) {
    uint orginst = inst;
    for(BDF::bdf_type bus = 0; bus < 256; bus++) {
        // skip empty busses
        //if (conf_read(bus << 8, 0) == ~0u) continue;
        for(BDF::bdf_type dev = 0; dev < 32; dev++) {
            BDF::bdf_type maxfunc = 1;
            for(BDF::bdf_type func = 0; func < maxfunc; func++) {
                BDF bdf(bus, dev, func);
                value_type value = read(bdf, 2 * 4);
                if(value == ~0U)
                    continue;
                if(maxfunc == 1 && (read(bdf, 3 * 4) & 0x800000))
                    maxfunc = 8;
                if((theclass == ~0U || ((value >> 24) & 0xff) == theclass)
                   && (subclass == ~0U || ((value >> 16) & 0xff) == subclass)
                   && (inst == ~0U || !inst--))
                    return bdf;
            }
        }
    }
    throw Exception(E_NOT_FOUND, 64, "Unable to find class %#x subclass %#x inst %#x",
                    theclass, subclass, orginst);
}

BDF HostPCIConfig::search_bridge(value_type dst) {
    value_type dstbus = dst >> 8;
    for(BDF::bdf_type dev = 0; dev < 32; dev++) {
        BDF::bdf_type maxfunc = 1;
        for(BDF::bdf_type func = 0; func < maxfunc; func++) {
            BDF bdf(0, dev, func);
            value_type value = read(bdf, 2 * 4);
            if(value == ~0U)
                continue;

            value_type header = read(bdf, 3 * 4) >> 16;
            if(maxfunc == 1 && (header & 0x80))
                maxfunc = 8;
            if((header & 0x7f) != 1)
                continue;

            // we have a bridge
            value_type b = read(bdf, 6 * 4);
            if((((b >> 8) & 0xff) <= dstbus) && (((b >> 16) & 0xff) >= dstbus))
                return bdf;
        }
    }
    throw Exception(E_NOT_FOUND, 32, "Unable to find bridge %#x", dst);
}
