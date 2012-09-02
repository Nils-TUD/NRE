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

#include <ipc/Service.h>
#include <services/PCIConfig.h>
#include <Logging.h>

#include "HostPCIConfig.h"
#include "HostMMConfig.h"

using namespace nre;

static HostPCIConfig *pcicfg;
static HostMMConfig *mmcfg;

static Config *find(PCIConfig::bdf_type bdf,size_t offset) {
	if(mmcfg && mmcfg->contains(bdf,offset))
		return mmcfg;
	if(pcicfg->contains(bdf,offset))
		return pcicfg;
	throw Exception(E_NOT_FOUND);
}

PORTAL static void portal_pcicfg(capsel_t) {
	UtcbFrameRef uf;
	try {
		PCIConfig::bdf_type bdf;
		size_t offset;
		PCIConfig::Command cmd;
		uf >> cmd;

		Config *cfg = 0;
		if(cmd == PCIConfig::READ || cmd == PCIConfig::WRITE || cmd == PCIConfig::ADDR) {
			uf >> bdf >> offset;
			cfg = find(bdf,offset);
		}

		switch(cmd) {
			case PCIConfig::READ: {
				uf.finish_input();
				PCIConfig::value_type res = cfg->read(bdf,offset);
				LOG(Logging::PCICFG,Serial::get().writef(
						"PCIConfig::READ bdf=%#x off=%#x: %#x\n",bdf,offset,res));
				uf << E_SUCCESS << res;
			}
			break;

			case PCIConfig::WRITE: {
				PCIConfig::value_type value;
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

			case PCIConfig::SEARCH_DEVICE: {
				PCIConfig::value_type theclass,subclass,inst;
				uf >> theclass >> subclass >> inst;
				uf.finish_input();
				PCIConfig::bdf_type bdf = pcicfg->search_device(theclass,subclass,inst);
				LOG(Logging::PCICFG,Serial::get().writef(
						"PCIConfig::SEARCH_DEVICE class=%#x subclass=%#x inst=%#x => %02x,%02x,%02x\n",
						theclass,subclass,inst,(bdf >> 8) & 0xFF,(bdf >> 3) & 0x1F,bdf & 0x7));
				uf << E_SUCCESS << bdf;
			}
			break;

			case PCIConfig::SEARCH_BRIDGE: {
				PCIConfig::value_type bridge;
				uf >> bridge;
				uf.finish_input();
				PCIConfig::bdf_type bdf = pcicfg->search_bridge(bridge);
				LOG(Logging::PCICFG,Serial::get().writef(
						"PCIConfig::SEARCH_BRIDGE bridge=%#x => %02x,%02x,%02x\n",
						bridge,(bdf >> 8) & 0xFF,(bdf >> 3) & 0x1F,bdf & 0x7));
				uf << E_SUCCESS << bdf;
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
		uf << e;
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

	Service *srv = new Service("pcicfg",CPUSet(CPUSet::ALL),portal_pcicfg);
	srv->reg();
	Sm sm(0);
	sm.down();
	return 0;
}
