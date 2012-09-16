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

#include <services/SysInfo.h>

#include "SysInfoService.h"
#include "Admission.h"
#include "PhysicalMemory.h"

using namespace nre;

void SysInfoService::portal(capsel_t) {
	UtcbFrameRef uf;
	SysInfoService *srv = Thread::current()->get_tls<SysInfoService*>(Thread::TLS_PARAM);
	try {
		SysInfo::Command cmd;
		uf >> cmd;

		switch(cmd) {
			case SysInfo::GET_MEM: {
				uf.finish_input();
				size_t total = PhysicalMemory::total_size();
				size_t free = PhysicalMemory::free_size();
				uf << E_SUCCESS << total << free;
			}
			break;

			case SysInfo::GET_TOTALTIME: {
				cpu_t cpu;
				bool update;
				uf >> cpu >> update;
				uf.finish_input();
				uf << E_SUCCESS << Admission::total_time(cpu,update);
			}
			break;

			case SysInfo::GET_TIMEUSER: {
				size_t idx;
				uf >> idx;
				uf.finish_input();

				String name;
				cpu_t cpu = 0;
				timevalue_t time = 0;
				bool res = Admission::get_sched_entity(idx,name,cpu,time);
				uf << E_SUCCESS;
				if(res)
					uf << true << name << cpu << time;
				else
					uf << false;
			}
			break;

			case SysInfo::GET_CHILD: {
				size_t idx;
				uf >> idx;
				uf.finish_input();

				if(idx >= ChildManager::MAX_CHILDS)
					throw Exception(E_ARGS_INVALID,32,"Invalid child index %zu",idx);

				size_t virt,phys;
				ScopedLock<RCULock> guard(&RCU::lock());
				const Child *c = srv->_cm->get_at(idx);
				if(c) {
					size_t threads = c->scs().length();
					c->reglist().memusage(virt,phys);

					uf << E_SUCCESS << true << c->cmdline() << virt << phys << threads;
				}
				else
					uf << E_SUCCESS << false;
			}
			break;
		}
	}
	catch(const Exception& e) {
		Syscalls::revoke(uf.delegation_window(),true);
		uf.clear();
		uf << e;
	}
}
