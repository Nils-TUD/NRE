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

#include <arch/Types.h>

#include "Timer.h"

class HostTimerDevice {
public:
	typedef uint64_t ticks_t;
	typedef uint64_t freq_t;

	explicit HostTimerDevice() {
	}
	virtual ~HostTimerDevice() {
	}

	virtual uint64_t last_ticks() = 0;
	virtual uint64_t current_ticks() = 0;
	virtual uint64_t update_ticks(bool refresh_only) = 0;

	virtual bool is_periodic() const = 0;
	virtual size_t timer_count() const = 0;
	virtual Timer *timer(size_t i) = 0;
	virtual bool is_in_past(uint64_t ticks) = 0;
	virtual uint64_t next_timeout(uint64_t now,uint64_t next) = 0;
	virtual void start(ticks_t ticks) = 0;
	virtual void enable(Timer *t,bool enable_ints) = 0;
	virtual void ack_irq(Timer *) {
	}
	virtual freq_t freq() = 0;
};
