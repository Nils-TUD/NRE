/*
 * TODO comment me
 *
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NUL.
 *
 * NUL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NUL is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#include <stream/Serial.h>
#include <Exception.h>

#include "HostHPET.h"
#include "HostPIT.h"

// Resolution of our TSC clocks per HPET clock measurement. Lower
// resolution mean larger error in HPET counter estimation.
#define CPT_RES   /* 1 divided by */ (1U<<13) /* clocks per hpet tick */

using namespace nul;

int main() {
	HostTimer *t;
	try {
		t = new HostHPET(false);
	}
	catch(const Exception &e) {
		Serial::get() << "TIMER: HPET initialization failed: " << e.msg() << "\n";
		Serial::get() << "TIMER: Trying PIT instead\n";
		t = new HostPIT(1000);
	}

    // HPET: Counter is running, IRQs are off.
    // PIT:  PIT is programmed to run in periodic mode, if HPET didn't work for us.

    uint64_t clocks_per_tick = (static_cast<uint64_t>(Hip::get().freq_tsc) * 1000 * CPT_RES) / t->freq();
	Serial::get().writef("TIMER: %Lu+%04Lu/%u TSC ticks per timer tick.\n",clocks_per_tick / CPT_RES,
			clocks_per_tick % CPT_RES,CPT_RES);

    // Get wallclock time
    HostTimer::ticks_t initial_counter = 0; // TODO wallclock_init();
    t->start(initial_counter);

	Sm sm(0);
	sm.down();
	return 0;
}
