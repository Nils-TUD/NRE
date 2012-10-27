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

#include <kobj/Pt.h>
#include <kobj/Sm.h>
#include <subsystem/ChildManager.h>
#include <mem/DataSpace.h>

using namespace nre;

int main() {
	// don't put it on the stack since its too large :)
	ChildManager *cm = new ChildManager();
	for(Hip::mem_iterator mem = Hip::get().mem_begin(); mem != Hip::get().mem_end(); ++mem) {
		if(strstr(mem->cmdline(), "bin/apps/test") != 0) {
			ChildConfig cfg(0, String(mem->cmdline()));
			DataSpace ds(mem->size, DataSpaceDesc::ANONYMOUS, DataSpaceDesc::R, mem->addr);
			cm->load(ds.virt(), mem->size, cfg);
			break;
		}
	}
	while(cm->count() > 0)
		cm->dead_sm().down();
	return 0;
}
