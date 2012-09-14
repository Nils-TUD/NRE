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

#include <stream/Serial.h>
#include <subsystem/ChildManager.h>
#include <Hip.h>

#include "VMConfig.h"

using namespace nre;

int main() {
	const Hip &hip = Hip::get();
	Serial::get() << "Hip is " << (hip.is_valid() ? "valid" : "not valid") << "\n";
	Serial::get() << "Modules:\n";
	for(Hip::mem_iterator mem = hip.mem_begin(); mem != hip.mem_end(); ++mem) {
		Serial::get().writef("  addr=%#Lx, size=%#Lx, cmdline='%s', type=%d\n",
				mem->addr,mem->size,mem->cmdline(),mem->type);
	}

	ChildManager *cm = new ChildManager();
	size_t i = 0;
	for(Hip::mem_iterator mem = hip.mem_begin(); mem != hip.mem_end(); ++mem, ++i) {
		if(strstr(mem->cmdline(),".vmconfig")) {
			VMConfig *cfg = new VMConfig(mem->addr,mem->size,mem->cmdline());
			cfg->start(cm);
		}
	}
	cm->wait_for_all();
	return 0;
}
