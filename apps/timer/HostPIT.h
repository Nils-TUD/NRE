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

#pragma once

#include <kobj/Ports.h>

#include "HostTimer.h"

class HostPIT : public HostTimer {
	enum {
		FREQ			= 1193180ULL,
		DEFAULT_PERIOD	= 1000ULL, // ms
		IRQ				= 2,
		PORT_BASE		= 0x40
	};

public:
	HostPIT(uint period_us);

	virtual void start(ticks_t ticks) {
		_ticks = ticks;
	}
	virtual freq_t freq() {
		return _freq;
	}

private:
	nul::Ports _ports;
	freq_t _freq;
	ticks_t _ticks;
};
