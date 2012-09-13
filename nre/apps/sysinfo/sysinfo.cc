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
#include <services/Console.h>
#include <services/Timer.h>
#include <services/SysInfo.h>
#include <util/Clock.h>

#define MAX_NAME_LEN	20
#define MAX_TIME_LEN	6
#define ROWS			(Console::ROWS - 4)

using namespace nre;

static Connection timercon("timer");
static Connection sysinfocon("sysinfo");
static SysInfoSession sysinfo(sysinfocon);
static Connection conscon("console");
static ConsoleSession cons(conscon,1);
static size_t top = 0;

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

static void refresh_console(bool update) {
	static UserSm sm;
	ScopedLock<UserSm> guard(&sm);
	cons.clear(0);
	ConsoleStream stream(cons,0);

	// display header
	size_t memtotal,memfree;
	sysinfo.get_mem(memtotal,memfree);
	stream.writef("%*s: %zu / %zu KiB\n",MAX_NAME_LEN,"Memory",memfree / 1024,memtotal / 1024);
	stream.writef("%*s: ",MAX_NAME_LEN,"CPU");
	for(CPU::iterator cpu = CPU::begin(); cpu != CPU::end(); ++cpu)
		stream.writef("%*u ",MAX_TIME_LEN - 1,cpu->log_id());
	stream.writef("\n");
	for(int i = 0; i < Console::COLS - 2; i++)
		stream << '-';
	stream << '\n';

	// determine the total time elapsed on each CPU. this way we don't assume that exactly 1sec
	// has passed since last update and are thus less dependend on the timer-service.
	static timevalue_t cputotal[Hip::MAX_CPUS];
	for(CPU::iterator cpu = CPU::begin(); cpu != CPU::end(); ++cpu)
		cputotal[cpu->log_id()] = sysinfo.get_total_time(cpu->log_id(),update);

	for(size_t idx = top, c = 0; c < ROWS; ++c, ++idx) {
		SysInfo::TimeUser tu;
		if(!sysinfo.get_timeuser(idx,tu))
			break;

		size_t namelen = 0;
		const char *name = getname(tu.name(),namelen);
		// writef doesn't support floats, so calculate it with 1000 as base and use the last decimal
		// for the first fraction-digit.
		timevalue_t permil;
		if(tu.time() == 0)
			permil = 0;
		else
			permil = (timevalue_t)(1000 / ((float)cputotal[tu.cpu()] / tu.time()));
		stream.writef("%*.*s: %*s%3Lu.%Lu\n",MAX_NAME_LEN,namelen,name,tu.cpu() * MAX_TIME_LEN,"",
				permil / 10,permil % 10);
	}
}

static void input_thread(void*) {
	size_t lasttop = top;
	while(1) {
		Console::ReceivePacket *pk = cons.consumer().get();
		if(!(pk->flags & Keyboard::RELEASE)) {
			switch(pk->keycode) {
				case Keyboard::VK_UP:
					if(top > 0)
						top--;
					break;
				case Keyboard::VK_DOWN:
					top++;
					break;
			}
		}
		cons.consumer().next();
		if(top != lasttop) {
			refresh_console(false);
			lasttop = top;
		}
	}
}

static void refresh_thread() {
	TimerSession timer(timercon);
	Clock clock(1000);
	while(1) {
		timevalue_t next = clock.source_time(1000);
		refresh_console(true);

		// wait a second
		timer.wait_until(next);
	}
}

int main() {
	GlobalThread gt(input_thread,CPU::current().log_id());
	Sc sc(&gt,Qpd());
	sc.start(String("sysinfo-input"));
	refresh_thread();
	return 0;
}
