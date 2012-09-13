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
#include <ipc/Connection.h>
#include <ipc/PtClientSession.h>
#include <utcb/UtcbFrame.h>
#include <Exception.h>
#include <CPU.h>

namespace nre {

/**
 * Types for the timer service
 */
class Timer {
public:
	enum {
		WALLCLOCK_FREQ	= 1000000
	};

	/**
	 * The available commands
	 */
	enum Command {
		GET_SMS,
		PROG_TIMER,
		GET_TIME
	};

private:
	Timer();
};

/**
 * Represents a session at the timer service
 */
class TimerSession : public PtClientSession {
public:
	/**
	 * Creates a new session with given connection
	 *
	 * @param con the connection
	 */
	explicit TimerSession(Connection &con) : PtClientSession(con) {
		get_sms();
	}
	/**
	 * Destroys this session
	 */
	virtual ~TimerSession() {
		for(cpu_t cpu = 0; cpu < CPU::count(); ++cpu)
			delete _sms[cpu];
		delete[] _sms;
		CapSelSpace::get().free(_caps,1 << CPU::order());
	}

	/**
	 * Returns the semaphore that is signaled as soon as a timer fires.
	 *
	 * @param cpu the CPU id
	 * @return the semaphore for the given CPU
	 */
	Sm &sm(cpu_t cpu) {
		return *_sms[cpu];
	}

	/**
	 * Waits for <cycles> cycles.
	 *
	 * @param cycles the number of cycles
	 */
	void wait_for(timevalue_t cycles) {
		wait_until(Util::tsc() + cycles);
	}

	/**
	 * Waits until the TSC has the value <cycles>
	 *
	 * @param cycles the number of cycles
	 */
	void wait_until(timevalue_t cycles) {
		program(cycles);
		_sms[CPU::current().log_id()]->zero();
	}

	/**
	 * Programs a timer for the TSC value <cycles>. That is, it does not block. Use sm(cpu_t) to
	 * get a signal.
	 *
	 * @param cycles the number of cycles
	 */
	void program(timevalue_t cycles) {
		UtcbFrame uf;
		uf << Timer::PROG_TIMER << cycles;
		pt().call(uf);
		uf.check_reply();
	}

	/**
	 * Determines the current time
	 *
	 * @param uptime the number of cycles since systemstart
	 * @param unixts the current unix timestamp in microseconds
	 */
	void get_time(timevalue_t &uptime,timevalue_t &unixts) {
		UtcbFrame uf;
		uf << Timer::GET_TIME;
		pt().call(uf);
		uf.check_reply();
		uf >> uptime >> unixts;
	}

private:
	void get_sms() {
		UtcbFrame uf;
		ScopedCapSels caps(1 << CPU::order(),1 << CPU::order());
		uf.delegation_window(Crd(caps.get(),CPU::order(),Crd::OBJ_ALL));
		uf << Timer::GET_SMS;
		pt().call(uf);
		uf.check_reply();
		_caps = caps.release();
		_sms = new Sm*[CPU::count()];
		for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it)
			_sms[it->log_id()] = new Sm(_caps + it->log_id(),true);
	}

	capsel_t _caps;
	Sm **_sms;
};

}
