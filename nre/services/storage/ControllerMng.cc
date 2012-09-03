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

#include <kobj/Gsi.h>

#include "ControllerMng.h"
#include "HostAHCI.h"
#include "HostIDE.h"

using namespace nre;

void ControllerMng::find_ahci_controller() {
	uint inst = 0;
	PCI::bdf_type bdf;
	while(_count < MAX_CONTROLLER) {
		try {
			bdf = _pcicfg.search_device(CLASS_STORAGE_CTRL,SUBCLASS_SATA,inst);
		}
		catch(const Exception &e) {
			LOG(Logging::STORAGE_DETAIL,Serial::get() << "Stopping search for SATA controllers: "
					<< e.code() << ": " << e.msg() << "\n");
			break;
		}

		//MessageHostOp msg1(MessageHostOp::OP_ASSIGN_PCI,bdf);
		// TODO bool dmar = mb.bus_hostop.send(msg1);
		bool dmar = false;
		Gsi *gsi = _pci.get_gsi(bdf,0);

		LOG(Logging::STORAGE,
				Serial::get().writef("Disk controller #%x AHCI (%02x,%02x,%02x) id %#x mmio %p\n",
				_count,(bdf >> 8) & 0xFF,(bdf >> 3) & 0x1F,bdf & 0x7,_pci.conf_read(bdf,0),
				_pci.conf_read(bdf,9)));

		_ctrls[_count++] = new HostAHCI(_pci,bdf,gsi,dmar);
		inst++;
	}
}

void ControllerMng::find_ide_controller() {
	uint inst = 0;
	PCI::bdf_type bdf;
	while(_count < MAX_CONTROLLER) {
		try {
			bdf = _pcicfg.search_device(CLASS_STORAGE_CTRL,SUBCLASS_IDE,inst);
		}
		catch(const Exception &e) {
			LOG(Logging::STORAGE_DETAIL,Serial::get() << "Stopping search for IDE controllers: "
					<< e.code() << ": " << e.msg() << "\n");
			break;
		}

		// primary and secondary controller
		for(uint i = 0; i < 2; i++) {
			PCI::value_type bar1 = _pci.conf_read(bdf,4 + i * 2);
			PCI::value_type bar2 = _pci.conf_read(bdf,4 + i * 2 + 1);

			// try legacy port
			if(!bar1 && !bar2) {
				if(!i) {
					bar1 = 0x1f1;
					bar2 = 0x3f7;
				}
				else {
					bar1 = 0x171;
					bar2 = 0x377;
				}
			}
			// we need both ports
			if(!(bar1 & bar2 & 1)) {
				LOG(Logging::STORAGE,Serial::get().writef("We need both ports: bar1=%#x, bar2=%#x\n",
						bar1,bar2));
				continue;
			}

			LOG(Logging::STORAGE,
					Serial::get().writef("Disk controller #%x IDE (%02x,%02x,%02x) iobase %#x\n",
					_count,(bdf >> 8) & 0xFF,(bdf >> 3) & 0x1F,bdf & 0x7,bar1 & ~0x3));

			// create controller
			_ctrls[_count++] = new HostIDE(bar1 & ~0x3);
		}
		inst++;
	}
}
