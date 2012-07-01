/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <kobj/Gsi.h>
#include <mem/DataSpace.h>
#include <util/Atomic.h>
#include <Assert.h>

#include "HostTimerDevice.h"

class HostHPET : public HostTimerDevice {
	enum {
		MAX_TIMERS				= 24,
		MIN_TICKS_BETWEEN_WRAP	= 4
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

	class HPETTimer : public Timer {
		friend class HostHPET;

	public:
		explicit HPETTimer() : Timer(), _no(), _gsi(), _reg() {
		}

		virtual nul::Gsi &gsi() {
			return *_gsi;
		}
		virtual void init(cpu_t cpu);
		virtual void program_timeout(timevalue_t next) {
			// Program a new timeout. Top 32-bits are discarded.
			_reg->comp[0] = next;
		}

	private:
		uint _no;
		uint _ack;
		nul::Gsi *_gsi;
		HostHpetTimer *_reg;
		static uint _assigned_irqs;
	};

public:
	explicit HostHPET(bool force_legacy);

	virtual timevalue_t last_ticks() {
		return nul::Atomic::read_atonce(_last);
	}
	virtual timevalue_t current_ticks() {
		uint32_t r = _reg->counter[0];
		return correct_overflow(nul::Atomic::read_atonce(_last),r);
	}
	virtual timevalue_t update_ticks(bool) {
		timevalue_t newv = _reg->counter[0];
		newv = correct_overflow(nul::Atomic::read_atonce(_last),newv);
		nul::Atomic::write_atonce(_last,newv);
		return newv;
	}

	virtual bool is_periodic() const {
		return false;
	}
	virtual size_t timer_count() const {
		return _usable_timers;
	}
	virtual Timer *timer(size_t i) {
		return _timer + i;
	}
	virtual timevalue_t freq() const {
		return _freq;
	}

	virtual bool is_in_past(timevalue_t ticks) const {
		return static_cast<int32_t>(ticks - _reg->counter[0]) <= 8;
	}
	virtual timevalue_t next_timeout(timevalue_t now,timevalue_t next) {
		// Generate at least some IRQs between wraparound IRQs to make
		// overflow detection robust. Only needed with HPETs.
		if(next == ~0ULL ||
				(static_cast<int64_t>(next - now) > 0x100000000LL / MIN_TICKS_BETWEEN_WRAP)) {
			next = now + 0x100000000ULL / MIN_TICKS_BETWEEN_WRAP;
		}
		return next;
	}
	virtual void start(timevalue_t ticks) {
	    assert((_reg->config & ENABLE_CNF) == 0);
		// Start HPET counter at value. HPET might be 32-bit. In this case,
		// the upper 32-bit of value are ignored.
	    _reg->main    = ticks;
	    _reg->config |= ENABLE_CNF;
	    _last = ticks;
	}
	virtual void enable(Timer *t,bool enable_ints) {
		HPETTimer *timer = static_cast<HPETTimer*>(t);
		if(enable_ints)
			timer->_reg->config |= INT_ENB_CNF;
	}
	virtual void ack_irq(Timer *t) {
		HPETTimer *timer = static_cast<HPETTimer*>(t);
		if(timer->_ack != 0)
			_reg->isr = timer->_ack;
	}

private:
	static timevalue_t correct_overflow(timevalue_t last,uint32_t newv) {
		bool of = (static_cast<uint32_t>(newv) < static_cast<uint32_t>(last));
		return (((last >> 32) + of) << 32) | newv;
	}

	static uintptr_t get_address();
	static uint16_t get_rid(uint8_t block,uint comparator);

	uintptr_t _addr;
	nul::DataSpace _mem;
	HostHpetRegister *_reg;
	uint _usable_timers;
	HPETTimer _timer[MAX_TIMERS];
	timevalue_t _freq;
	volatile timevalue_t _last;
	static bool _verbose;
};
