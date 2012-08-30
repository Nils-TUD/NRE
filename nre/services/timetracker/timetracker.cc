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

#define MAX_NAME_LEN	20
#define MAX_TIME_LEN	6
#define ROWS			(Console::ROWS - 3)
#define MS_PER_SEC		(1000 * 1000)

using namespace nre;

class TimeTrackerServiceSession : public ServiceSession {
public:
	explicit TimeTrackerServiceSession(Service *s,size_t id,capsel_t caps,Pt::portal_func func)
		: ServiceSession(s,id,caps,func), _top(0), _lastcputotal(), _users(), _cons_con("console"),
		  _cons_sess(_cons_con,1) {
	}

	size_t top() const {
		return _top;
	}
	void top(size_t top) {
		_top = top;
	}

	timevalue_t last_cpu_total(cpu_t cpu) const {
		return _lastcputotal[cpu];
	}
	void last_cpu_total(cpu_t cpu,timevalue_t val) {
		_lastcputotal[cpu] = val;
	}

	ConsoleSession &console() {
		return _cons_sess;
	}
	const SList<TimeUser> &users() {
		return _users;
	}

	void start(const String &name,cpu_t cpu,capsel_t sc,bool idle) {
		ScopedLock<UserSm> guard(&sm);
		_users.append(new TimeUser(name,cpu,sc,idle));
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
	size_t _top;
	timevalue_t _lastcputotal[Hip::MAX_CPUS];
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
				bool idle;
				capsel_t cap = uf.get_delegated(0).offset();
				uf >> name >> cpu >> idle;
				uf.finish_input();

				sess->start(name,cpu,cap,idle);
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

static const char *getname(const String &name,size_t &len) {
	// don't display the path to the program (might be long) and cut off arguments
	size_t lastslash = 0,end = name.length();
	for(size_t i = 0; i < name.length(); ++i) {
		if(name.str()[i] == '/')
			lastslash = i + 1;
		if(name.str()[i] == ' ') {
			end = i;
			break;
		}
	}
	len = end - lastslash;
	return name.str() + lastslash;
}

static void refresh_console(TimeTrackerService::iterator sess,ConsoleSession &cons,bool update) {
	static UserSm sm;
	ScopedLock<UserSm> guard(&sm);
	cons.clear(0);
	ConsoleStream stream(cons,0);

	// display header
	stream.writef("%*s: ",MAX_NAME_LEN,"CPU");
	for(CPU::iterator cpu = CPU::begin(); cpu != CPU::end(); ++cpu)
		stream.writef("%*u",MAX_TIME_LEN,cpu->log_id());
	stream.writef("\n");
	for(int i = 0; i < Console::COLS - 2; i++)
		stream << '-';
	stream << '\n';

	// determine the total time elapsed on each CPU. this way we don't assume that exactly 1sec
	// has passed since last update and are thus less dependend on the timer-service.
	static timevalue_t cputotal[Hip::MAX_CPUS];
	SList<TimeUser> users = sess->users();
	for(SList<TimeUser>::iterator u = users.begin(); u != users.end(); ++u)
		cputotal[u->cpu()] += u->ms_last_sec(update);

	size_t y = 0,c = 0;
	for(SList<TimeUser>::iterator u = users.begin(); c < ROWS && u != users.end(); ++u, ++y) {
		if(y < sess->top())
			continue;

		// don't update the time again
		timevalue_t time = u->ms_last_sec(false);
		size_t namelen = 0;
		const char *name = getname(u->name(),namelen);
		// determine percentage of time used in the last second on this CPU
		timevalue_t diff = cputotal[u->cpu()] - sess->last_cpu_total(u->cpu());
		// writef doesn't support floats, so calculate it with 1000 as base and use the last decimal
		// for the first fraction-digit.
		timevalue_t permil = (timevalue_t)(1000 / ((float)diff / time));
		stream.writef("%*.*s: %*s%3Lu.%Lu\n",MAX_NAME_LEN,namelen,name,u->cpu() * MAX_TIME_LEN,"",
				permil / 10,permil % 10);
		c++;
	}

	// store last cpu total and reset value
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it) {
		sess->last_cpu_total(cputotal[it->log_id()]);
		cputotal[it->log_id()] = 0;
	}
}

static void input_thread(void*) {
	Connection timercon("timer");
	TimerSession timer(timercon);
	Clock clock(1000);
	while(1) {
		{
			ScopedLock<RCULock> guard(&RCU::lock());
			TimeTrackerService::iterator it(srv);
			for(it = srv->sessions_begin(); it != srv->sessions_end(); ++it) {
				ConsoleSession &cons = it->console();
				bool changed = false;
				while(cons.consumer().has_data()) {
					Console::ReceivePacket *pk = cons.consumer().get();
					if(pk->flags & Keyboard::RELEASE) {
						switch(pk->keycode) {
							case Keyboard::VK_UP:
								if(it->top() > 0) {
									it->top(it->top() - 1);
									changed = true;
								}
								break;
							case Keyboard::VK_DOWN:
								if(it->users().length() > ROWS + it->top()) {
									it->top(it->top() + 1);
									changed = true;
								}
								break;
						}
					}
					cons.consumer().next();
				}

				if(changed)
					refresh_console(it,cons,false);
			}
		}

		// wait a bit
		timer.wait_until(clock.source_time(50));
	}
}

static void refresh_thread() {
	Connection timercon("timer");
	TimerSession timer(timercon);
	Clock clock(1000);
	int i = 0;
	while(1) {
		timevalue_t next = clock.source_time(1000);
		{
			ScopedLock<RCULock> guard(&RCU::lock());
			TimeTrackerService::iterator it(srv);
			for(it = srv->sessions_begin(); it != srv->sessions_end(); ++it) {
				ConsoleSession &cons = it->console();
				refresh_console(it,cons,true);
			}
		}

		// wait a second
		timer.wait_until(next);
		i++;
	}
}

int main() {
	srv = new TimeTrackerService("timetracker");

	GlobalThread gt(input_thread,CPU::current().log_id());
	Sc sc(&gt,Qpd());
	sc.start(String("timetracker-input"));

	srv->reg();
	refresh_thread();
	return 0;
}
