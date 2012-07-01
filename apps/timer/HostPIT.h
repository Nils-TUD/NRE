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
#include <kobj/Gsi.h>

#include "HostTimerDevice.h"

class HostPIT : public HostTimerDevice {
	enum {
		FREQ			= 1193180ULL,
		DEFAULT_PERIOD	= 1000ULL, // ms
		IRQ				= 2,
		PORT_BASE		= 0x40
	};

	class PitTimer : public Timer {
	public:
		explicit PitTimer() : Timer(), _gsi(IRQ) {
		}

		virtual nul::Gsi &gsi() {
			return _gsi;
		}
		virtual void init(cpu_t) {
		}
		virtual void program_timeout(uint64_t) {
		}

	private:
		nul::Gsi _gsi;
	};

public:
	explicit HostPIT(uint period_us);

	virtual uint64_t last_ticks() {
		return nul::Atomic::read_atonce(_ticks);
	}
	virtual uint64_t current_ticks() {
		return nul::Atomic::read_atonce(_ticks);
	}
	virtual uint64_t update_ticks(bool refresh_only) {
		return refresh_only ? _ticks : ++_ticks;
	}

	virtual bool is_periodic() const {
		return true;
	}
	virtual size_t timer_count() const {
		return 1;
	}
	virtual Timer *timer(size_t) {
		return &_timer;
	}
	virtual freq_t freq() {
		return _freq;
	}

	virtual bool is_in_past(uint64_t ticks) {
		return ticks < _ticks;
	}
	virtual uint64_t next_timeout(uint64_t,uint64_t next) {
		return next;
	}
	virtual void start(ticks_t ticks) {
		_ticks = ticks;
	}
	virtual void enable(Timer *,bool) {
		// nothing to do
	}

private:
	nul::Ports _ports;
	freq_t _freq;
	ticks_t _ticks;
	PitTimer _timer;
};
