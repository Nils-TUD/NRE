/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2011, Julian Stecklina <jsteckli@os.inf.tu-dresden.de>
 * Copyright (C) 2010, Bernhard Kauer <bk@vmmon.org>
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

#include <stream/Serial.h>
#include <Logging.h>

#include "HostPIT.h"

using namespace nre;

HostPIT::HostPIT(uint period_us) : _ports(PORT_BASE, 4), _freq() {
    timevalue_t value = (FREQ * period_us) / 1000000ULL;
    if(value == 0 || value > 65535) {
        LOG(TIMER, "TIMER: Bogus PIT period " << period_us << "us."
                                              << " Set to default (" << DEFAULT_PERIOD << "us)\n");
        period_us = DEFAULT_PERIOD;
        value = (FREQ * DEFAULT_PERIOD) / 1000000ULL;
    }

    _ports.out<uint8_t>(0x34, 3);
    _ports.out<uint8_t>(value, 0);
    _ports.out<uint8_t>(value >> 8, 0);
    _freq = 1000000U / period_us;

    LOG(TIMER, "TIMER: PIT initalized. Ticks every "
               << period_us << "us" << " (period " << value <<", " << _freq << "HZ).\n");
}
