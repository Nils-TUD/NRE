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
#include <ipc/Session.h>
#include <utcb/UtcbFrame.h>
#include <Exception.h>
#include <CPU.h>

namespace nre {

class Timer {
public:
	enum {
		WALLCLOCK_FREQ	= 1000000
	};

	enum Command {
		GET_SMS,
		PROG_TIMER,
		GET_TIME
	};

private:
	Timer();
};

class TimerSession : public Session {
public:
	explicit TimerSession(Connection &con) : Session(con), _pts(new Pt*[CPU::count()]) {
		for(cpu_t cpu = 0; cpu < CPU::count(); ++cpu)
			_pts[cpu] = con.available_on(cpu) ? new Pt(caps() + cpu) : 0;
		get_sms();
	}
	virtual ~TimerSession() {
		for(cpu_t cpu = 0; cpu < CPU::count(); ++cpu) {
			delete _sms[cpu];
			delete _pts[cpu];
		}
		delete[] _sms;
		delete[] _pts;
		CapSpace::get().free(_caps,Math::next_pow2(CPU::count()));
	}

	Sm &sm(cpu_t cpu) {
		return *_sms[cpu];
	}

	void wait_for(timevalue_t cycles) {
		wait_until(Util::tsc() + cycles);
	}

	void wait_until(timevalue_t cycles) {
		program(cycles);
		_sms[CPU::current().log_id()]->zero();
	}

	void program(timevalue_t cycles) {
		UtcbFrame uf;
		uf << Timer::PROG_TIMER << cycles;
		_pts[CPU::current().log_id()]->call(uf);
		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw Exception(res);
	}

	void get_time(timevalue_t &uptime,timevalue_t &unixts) {
		UtcbFrame uf;
		uf << Timer::GET_TIME;
		_pts[CPU::current().log_id()]->call(uf);
		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw Exception(res);
		uf >> uptime >> unixts;
	}

private:
	void get_sms() {
		UtcbFrame uf;
		uint order = Math::next_pow2_shift(CPU::count());
		ScopedCapSels caps(1 << order,1 << order);
		uf.delegation_window(Crd(caps.get(),order,Crd::OBJ_ALL));
		uf << Timer::GET_SMS;
		_pts[CPU::current().log_id()]->call(uf);
		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw Exception(res);
		_caps = caps.release();
		_sms = new Sm*[CPU::count()];
		for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it)
			_sms[it->log_id()] = new Sm(_caps + it->log_id(),true);
	}

	capsel_t _caps;
	Sm **_sms;
	Pt **_pts;
};

}
