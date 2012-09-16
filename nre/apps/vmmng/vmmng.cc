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

#include <subsystem/ChildManager.h>
#include <services/Console.h>
#include <services/Timer.h>
#include <stream/Serial.h>
#include <stream/ConsoleStream.h>
#include <util/Clock.h>
#include <util/Cycler.h>
#include <Hip.h>

#include "VMConfig.h"
#include "RunningVM.h"
#include "RunningVMList.h"
#include "VMMngService.h"

using namespace nre;

enum {
	CUR_ROW_COLOR	= 0x70
};

static size_t vmidx = 0;
static Connection conscon("console");
static ConsoleSession cons(conscon,0,String("VMManager"));
static SList<VMConfig> configs;
static ChildManager cm;
static Cycler<CPU::iterator> cpucyc(CPU::begin(),CPU::end());

static void refresh_console() {
	static UserSm sm;
	ScopedLock<UserSm> guard(&sm);
	ConsoleStream cs(cons,0);
	cons.clear(0);
	cs << "Welcome to the interactive VM manager!\n\n";
	cs << "VM configurations:\n";
	size_t no = 1;
	for(SList<VMConfig>::iterator it = configs.begin(); it != configs.end(); ++it, ++no)
		cs << "  [" << no << "] " << it->name() << "\n";
	cs << "\n";

	cs << "Running VMs:\n";
	ScopedLock<RCULock> rcuguard(&RCU::lock());
	RunningVM *vm;
	if(vmidx >= RunningVMList::get().count())
		vmidx = RunningVMList::get().count() - 1;
	for(size_t i = 0; (vm = RunningVMList::get().get(i)) != 0; ++i) {
		try {
			const Child &c = cm.get(vm->id());
			uint8_t oldcol = cs.color();
			if(vmidx == i)
				cs.color(CUR_ROW_COLOR);
			cs << "  [" << (i + 1) << "] on CPU " << c.cpu() << ": " << vm->cfg()->args();
			while(cs.x() != 0)
				cs << ' ';
			if(vmidx == i)
				cs.color(oldcol);
		}
		catch(...) {
		}
	}
	cs << "\nPress R to reset or K to kill the selected VM";
}

static void input_thread(void*) {
	while(1) {
		Console::ReceivePacket *pk = cons.consumer().get();
		switch(pk->keycode) {
			case Keyboard::VK_1...Keyboard::VK_9: {
				if(pk->flags & Keyboard::RELEASE) {
					size_t idx = pk->keycode - Keyboard::VK_1;
					SList<VMConfig>::iterator it;
					for(it = configs.begin(); it != configs.end() && idx-- > 0; ++it)
						;
					if(it != configs.end())
						RunningVMList::get().add(cm,&*it,cpucyc.next()->log_id());
				}
			}
			break;

			case Keyboard::VK_R: {
				ScopedLock<RCULock> guard(&RCU::lock());
				RunningVM *vm = RunningVMList::get().get(vmidx);
				if(vm && (pk->flags & Keyboard::RELEASE)) {
					if(pk->keycode == Keyboard::VK_R)
						vm->execute(VMManager::RESET);
					else
						vm->execute(VMManager::KILL);
				}
			}
			break;

			case Keyboard::VK_UP:
				if((~pk->flags & Keyboard::RELEASE) && vmidx > 0) {
					vmidx--;
					refresh_console();
				}
				break;

			case Keyboard::VK_DOWN:
				if((~pk->flags & Keyboard::RELEASE) && vmidx + 1 < RunningVMList::get().count()) {
					vmidx++;
					refresh_console();
				}
				break;

			case Keyboard::VK_K: {
				if(pk->flags & Keyboard::RELEASE) {
					Child::id_type id = ObjCap::INVALID;
					{
						ScopedLock<RCULock> guard(&RCU::lock());
						RunningVM *vm = RunningVMList::get().get(vmidx);
						if(vm) {
							id = vm->id();
							RunningVMList::get().remove(vm);
						}
					}
					if(id != ObjCap::INVALID)
						cm.kill(id);
				}
			}
			break;
		}
		cons.consumer().next();
	}
}

static void refresh_thread() {
	Connection timercon("timer");
	TimerSession timer(timercon);
	Clock clock(1000);
	while(1) {
		timevalue_t next = clock.source_time(1000);
		refresh_console();

		// wait a second
		timer.wait_until(next);
	}
}

int main() {
	const Hip &hip = Hip::get();

	for(Hip::mem_iterator mem = hip.mem_begin(); mem != hip.mem_end(); ++mem) {
		if(strstr(mem->cmdline(),".vmconfig")) {
			VMConfig *cfg = new VMConfig(mem->addr,mem->size,mem->cmdline());
			configs.append(cfg);
			Serial::get() << *cfg << "\n";
		}
	}

	GlobalThread *gt = new GlobalThread(input_thread,CPU::current().log_id());
	Sc *sc = new Sc(gt,Qpd());
	sc->start(String("vmmng-input"));

	VMMngService *srv = VMMngService::create("vmmanager");
	srv->reg();

	refresh_thread();
	return 0;
}
