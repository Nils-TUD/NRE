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

#include <kobj/Sc.h>
#include <ipc/Service.h>
#include <stream/ConsoleStream.h>
#include <services/TimeTracker.h>
#include <services/Console.h>
#include <services/Timer.h>
#include <util/Clock.h>
#include "TimeUser.h"

using namespace nre;

class TimeTrackerServiceSession : public ServiceSession {
public:
	explicit TimeTrackerServiceSession(Service *s,size_t id,capsel_t caps,Pt::portal_func func)
		: ServiceSession(s,id,caps,func), _users(), _cons_con("console"), _cons_sess(_cons_con,1) {
	}

	ConsoleSession &console() {
		return _cons_sess;
	}
	const SList<TimeUser> &users() {
		return _users;
	}

	void start(const String &name,cpu_t cpu,capsel_t sc) {
		ScopedLock<UserSm> guard(&sm);
		_users.append(new TimeUser(name,cpu,sc));
	}

	void stop(capsel_t sc,cpu_t) {
		ScopedLock<UserSm> guard(&sm);
		for(SList<TimeUser>::iterator it = _users.begin(); it != _users.end(); ++it) {
			if(it->cap() == sc) {
				_users.remove(&*it);
				delete &*it;
				break;
			}
		}
	}

	static UserSm sm;
private:
	SList<TimeUser> _users;
	Connection _cons_con;
	ConsoleSession _cons_sess;
};

class TimeTrackerService : public Service {
public:
	typedef SessionIterator<TimeTrackerServiceSession> iterator;

	explicit TimeTrackerService(const char *name)
			: Service(name,CPUSet(CPUSet::ALL),portal) {
		// we want to accept one Sc, delegated and translated
		for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it) {
			LocalThread *ec = get_thread(it->log_id());
			UtcbFrameRef uf(ec->utcb());
			uf.accept_delegates(0);
			uf.accept_translates();
		}
	}

	iterator sessions_begin() {
		return Service::sessions_begin<TimeTrackerServiceSession>();
	}
	iterator sessions_end() {
		return Service::sessions_end<TimeTrackerServiceSession>();
	}

private:
	PORTAL static void portal(capsel_t pid);

	virtual ServiceSession *create_session(size_t id,capsel_t caps,Pt::portal_func func) {
		return new TimeTrackerServiceSession(this,id,caps,func);
	}
};

UserSm TimeTrackerServiceSession::sm;
static TimeTrackerService *srv;

void TimeTrackerService::portal(capsel_t pid) {
	UtcbFrameRef uf;
	try {
		ScopedLock<RCULock> guard(&RCU::lock());
		TimeTrackerServiceSession *sess = srv->get_session<TimeTrackerServiceSession>(pid);
		TimeTracker::Command cmd;
		uf >> cmd;

		switch(cmd) {
			case TimeTracker::START: {
				String name;
				cpu_t cpu;
				capsel_t cap = uf.get_delegated(0).offset();
				uf >> name >> cpu;
				uf.finish_input();

				sess->start(name,cpu,cap);
				uf.accept_delegates();
				uf << E_SUCCESS;
			}
			break;

			case TimeTracker::STOP: {
				cpu_t cpu;
				capsel_t cap = uf.get_translated(0).offset();
				uf >> cpu;
				uf.finish_input();

				sess->stop(cap,cpu);
				uf << E_SUCCESS;
			}
			break;
		}
	}
	catch(const Exception &e) {
		Syscalls::revoke(uf.delegation_window(),true);
		uf.clear();
		uf << e.code();
	}
}

static void refresh_thread() {
	Connection timercon("timer");
	TimerSession timer(timercon);
	Clock clock(1000);
	int i = 0;
	while(1) {
		{
			ScopedLock<RCULock> guard(&RCU::lock());
			TimeTrackerService::iterator it(srv);
			for(it = srv->sessions_begin(); it != srv->sessions_end(); ++it) {
				ConsoleSession &cons = it->console();
				cons.clear(0);
				ConsoleStream stream(cons,0);

				stream.writef("%20s  ","CPUs");
				for(CPU::iterator cpu = CPU::begin(); cpu != CPU::end(); ++cpu)
					stream.writef("%-12u",cpu->log_id());
				stream.writef("\n");
				for(int i = 0; i < Console::COLS; i++)
					stream << '-';

				SList<TimeUser> users = it->users();
				for(SList<TimeUser>::iterator u = users.begin(); u != users.end(); ++u) {
					timevalue_t time = Syscalls::sc_time(u->cap());
					stream.writef("%20.20s: %*s%Lu\n",u->name().str(),u->cpu() * 12,"",time);
				}
			}
		}

		// wait a second
		timer.wait_until(clock.source_time(1000));
		i++;
	}
}

int main() {
	srv = new TimeTrackerService("timetracker");
	srv->reg();
	refresh_thread();
	return 0;
}
