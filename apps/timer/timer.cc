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

#include <kobj/Sm.h>
#include <dev/Timer.h>

#include "HostTimer.h"

using namespace nul;

class TimerService;

static HostTimer *timer;
static TimerService *srv;

class TimerSessionData : public SessionData {
public:
	TimerSessionData(Service *s,size_t id,capsel_t caps,Pt::portal_func func)
		: SessionData(s,id,caps,func), _data(new HostTimer::ClientData[CPU::count()]) {
		for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it)
			timer->setup_clientdata(_data + it->log_id(),it->log_id());
	}
	virtual ~TimerSessionData() {
		delete[] _data;
	}

	HostTimer::ClientData *data(cpu_t cpu) {
		return _data + cpu;
	}

private:
	HostTimer::ClientData *_data;
};

class TimerService : public Service {
public:
	TimerService(const char *name,Pt::portal_func func) : Service(name,func) {
	}

private:
	virtual SessionData *create_session(size_t id,capsel_t caps,Pt::portal_func func) {
		return new TimerSessionData(this,id,caps,func);
	}
};

PORTAL static void portal_timer(capsel_t pid) {
	ScopedLock<RCULock> guard(&RCU::lock());
	TimerSessionData *sess = srv->get_session<TimerSessionData>(pid);
	UtcbFrameRef uf;
	try {
		nul::Timer::Command cmd;
		uf >> cmd;

		switch(cmd) {
			case nul::Timer::GET_SMS:
				uf.finish_input();

				for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it)
					uf.delegate(sess->data(it->log_id())->sm->sel(),it->log_id());
				uf << E_SUCCESS;
				break;

			case nul::Timer::PROG_TIMER: {
				timevalue_t time;
				uf >> time;
				uf.finish_input();

				timer->program_timer(sess->data(CPU::current().log_id()),time);
				uf << E_SUCCESS;
			}
			break;

			case nul::Timer::GET_TIME: {
				uf.finish_input();

				timevalue_t uptime,unixts;
		        timer->get_time(uptime,unixts);
		        uf << E_SUCCESS << uptime << unixts;
			}
			break;
		}
	}
	catch(const Exception &e) {
		uf.clear();
		uf << e.code();
	}
}

int main() {
	timer = new HostTimer();
	srv = new TimerService("timer",portal_timer);
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it)
		srv->provide_on(it->log_id());
	srv->reg();

	Sm sm(0);
	sm.down();
	return 0;
}
