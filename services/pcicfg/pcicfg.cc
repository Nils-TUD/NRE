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

#include <ipc/Service.h>
#include <services/PCIConfig.h>
#include <Logging.h>

#include "HostPCIConfig.h"
#include "HostMMConfig.h"

using namespace nre;

static HostPCIConfig *pcicfg;
static HostMMConfig *mmcfg;

static Config *find(uint32_t bdf,size_t offset) {
	if(mmcfg && mmcfg->contains(bdf,offset))
		return mmcfg;
	if(pcicfg->contains(bdf,offset))
		return pcicfg;
	throw Exception(E_NOT_FOUND);
}

PORTAL static void portal_pcicfg(capsel_t) {
	UtcbFrameRef uf;
	try {
		uint32_t bdf;
		size_t offset;
		PCIConfig::Command cmd;
		uf >> cmd;

		Config *cfg = 0;
		if(cmd != PCIConfig::REBOOT) {
			uf >> bdf >> offset;
			cfg = find(bdf,offset);
		}

		switch(cmd) {
			case PCIConfig::READ: {
				uf.finish_input();
				uint32_t res = cfg->read(bdf,offset);
				LOG(Logging::PCICFG,Serial::get().writef(
						"PCIConfig::READ bdf=%#x off=%#x: %#x\n",bdf,offset,res));
				uf << E_SUCCESS << res;
			}
			break;

			case PCIConfig::WRITE: {
				uint32_t value;
				uf >> value;
				uf.finish_input();
				cfg->write(bdf,offset,value);
				LOG(Logging::PCICFG,Serial::get().writef(
						"PCIConfig::WRITE bdf=%#x off=%#x: %#x\n",bdf,offset,value));
				uf << E_SUCCESS;
			}
			break;

			case PCIConfig::ADDR: {
				uf.finish_input();
				uintptr_t res = cfg->addr(bdf,offset);
				LOG(Logging::PCICFG,Serial::get().writef(
						"PCIConfig::ADDR bdf=%#x off=%#x: %p\n",bdf,offset,res));
				uf << E_SUCCESS << res;
			}
			break;

			case PCIConfig::REBOOT: {
				uf.finish_input();
				pcicfg->reset();
				uf << E_SUCCESS;
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
	pcicfg = new HostPCIConfig();
	try {
		mmcfg = new HostMMConfig();
	}
	catch(const Exception &e) {
		Serial::get() << e.name() << ": " << e.msg() << "\n";
	}

	Service *srv = new Service("pcicfg",portal_pcicfg);
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it)
		srv->provide_on(it->log_id());
	srv->reg();

	Sm sm(0);
	sm.down();
	return 0;
}
