/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <mem/DataSpace.h>
#include <Assert.h>

#include "HostTimer.h"

class HostHPET : public HostTimer {
	enum {
		MAX_TIMERS		= 24
	};

	enum {
		// General Configuration Register
		ENABLE_CNF		= (1U << 0),
		LEG_RT_CNF		= (1U << 1),

		// General Capabilities Register
		LEG_RT_CAP		= (1U << 15),
		BIT64_CAP		= (1U << 13),

		// Timer Configuration
		FSB_INT_DEL_CAP	= (1U << 15),
		FSB_INT_EN_CNF	= (1U << 14),

		MODE32_CNF		= (1U << 8),
		PER_INT_CAP		= (1U << 4),
		TYPE_CNF		= (1U << 3),
		INT_ENB_CNF		= (1U << 2),
		INT_TYPE_CNF	= (1U << 1),
	};

	struct HostHpetTimer {
		volatile uint32_t config;
		volatile uint32_t int_route;
		union {
			volatile uint32_t comp[2];
			volatile uint64_t comp64;
		};
		volatile uint32_t msi[2];
		uint32_t res[2];
	};

	struct HostHpetRegister {
		volatile uint32_t cap;
		volatile uint32_t period;
		uint32_t res0[2];
		volatile uint32_t config;
		uint32_t res1[3];
		volatile uint32_t isr;
		uint32_t res2[51];
		union {
			volatile uint32_t counter[2];
			volatile uint64_t main;
		};
		uint32_t res3[2];
		HostHpetTimer timer[24];
	};

	struct Timer {
		unsigned _no;
		HostHpetTimer *_reg;

		Timer() : _no(), _reg() {
		}
	};

public:
	HostHPET(bool force_legacy);

	virtual void start(ticks_t ticks) {
	    assert((_reg->config & ENABLE_CNF) == 0);
		// Start HPET counter at value. HPET might be 32-bit. In this case,
		// the upper 32-bit of value are ignored.
	    _reg->main    = ticks;
	    _reg->config |= ENABLE_CNF;
	}
	virtual freq_t freq() {
		return _freq;
	}

private:
	static uintptr_t get_address();

	uintptr_t _addr;
	nul::DataSpace _mem;
	HostHpetRegister *_reg;
	uint _usable_timers;
	Timer _timer[MAX_TIMERS];
	freq_t _freq;
	static bool _verbose;
};
