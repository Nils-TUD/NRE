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
#include <kobj/Gsi.h>

class HostTimerDevice {
public:
	class Timer {
	public:
		explicit Timer() {
		}
		virtual ~Timer() {
		}

		virtual nre::Gsi &gsi() = 0;
		virtual void init(cpu_t cpu) = 0;
		virtual void program_timeout(timevalue_t next) = 0;
	};

	explicit HostTimerDevice() {
	}
	virtual ~HostTimerDevice() {
	}

	virtual timevalue_t last_ticks() = 0;
	virtual timevalue_t current_ticks() = 0;
	virtual timevalue_t update_ticks(bool refresh_only) = 0;

	virtual bool is_periodic() const = 0;
	virtual size_t timer_count() const = 0;
	virtual Timer *timer(size_t i) = 0;
	virtual bool is_in_past(timevalue_t ticks) const = 0;
	virtual timevalue_t next_timeout(timevalue_t now,timevalue_t next) = 0;
	virtual void start(timevalue_t ticks) = 0;
	virtual void enable(Timer *t,bool enable_ints) = 0;
	virtual void ack_irq(Timer *) {
	}
	virtual timevalue_t freq() const = 0;
};
