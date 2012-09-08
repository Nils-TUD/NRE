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

#include "ConsoleSessionData.h"
#include "ConsoleService.h"
#include "Keymap.h"

using namespace nre;

int main() {
	ConsoleService *srv = new ConsoleService("console");
	srv->reg();
	Connection con("keyboard");
	KeyboardSession kb(con);
	for(Keyboard::Packet *pk; (pk = kb.consumer().get()) != 0; kb.consumer().next()) {
		if(!srv->handle_keyevent(*pk)) {
			ScopedLock<RCULock> guard(&RCU::lock());
			ConsoleSessionData *sess = srv->active();
			if(sess && sess->prod()) {
				Console::ReceivePacket rpk;
				rpk.flags = pk->flags;
				rpk.scancode = pk->scancode;
				rpk.keycode = pk->keycode;
				rpk.character = Keymap::translate(*pk);
				sess->prod()->produce(rpk);
			}
		}
	}
	return 0;
}
