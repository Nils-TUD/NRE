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

#include "SysInfoPage.h"

using namespace nre;

static Connection timercon("timer");
static Connection sysinfocon("sysinfo");
static SysInfoSession sysinfo(sysinfocon);
static Connection conscon("console");
static ConsoleSession cons(conscon, 0, String("SysInfo"));
static size_t page = 0;
static SysInfoPage *pages[] = {
	new ScInfoPage(cons, sysinfo),
	new PdInfoPage(cons, sysinfo)
};

static void input_thread(void*) {
	while(1) {
		bool changed = false, update = false;
		Console::ReceivePacket *pk = cons.consumer().get();
		if(!(pk->flags & Keyboard::RELEASE)) {
			switch(pk->keycode) {
				case Keyboard::VK_UP:
					if(pages[page]->top() > 0) {
						pages[page]->top(pages[page]->top() - 1);
						changed = true;
					}
					break;
				case Keyboard::VK_DOWN:
					pages[page]->top(pages[page]->top() + 1);
					changed = true;
					break;
				case Keyboard::VK_PGUP:
					if(pages[page]->top() >= SysInfoPage::ROWS)
						pages[page]->top(pages[page]->top() - SysInfoPage::ROWS);
					else
						pages[page]->top(0);
					changed = true;
					break;
				case Keyboard::VK_PGDOWN:
					pages[page]->top(pages[page]->top() + SysInfoPage::ROWS);
					changed = true;
					break;
				case Keyboard::VK_LEFT:
					page = (page - 1) % ARRAY_SIZE(pages);
					changed = update = true;
					break;
				case Keyboard::VK_RIGHT:
					page = (page + 1) % ARRAY_SIZE(pages);
					changed = update = true;
					break;
			}
		}
		cons.consumer().next();
		if(changed)
			pages[page]->refresh_console(update);
	}
}

static void refresh_thread() {
	TimerSession timer(timercon);
	Clock clock(1000);
	while(1) {
		timevalue_t next = clock.source_time(1000);
		pages[page]->refresh_console(true);

		// wait a second
		timer.wait_until(next);
	}
}

int main() {
	// disable cursor
	Console::Register regs = cons.get_regs();
	regs.cursor_style = 0x2000;
	cons.set_regs(regs);

	GlobalThread::create(input_thread, CPU::current().log_id(), String("sysinfo-input"))->start();
	refresh_thread();
	return 0;
}
