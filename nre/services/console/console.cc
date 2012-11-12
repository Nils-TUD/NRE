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

#include <stream/IStringStream.h>

#include "ConsoleSessionData.h"
#include "ConsoleService.h"
#include "Keymap.h"

using namespace nre;

static ConsoleService *srv;

static void input_thread(void*) {
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
}

int main(int argc, char *argv[]) {
    uint modifier = Keyboard::LCTRL;
    for(int i = 1; i < argc; ++i) {
        if(strncmp(argv[i], "modifier=", 9) == 0)
            modifier = 1 << IStringStream::read_from<uint>(argv[i] + 9, strlen(argv[i] + 9));
    }

    srv = new ConsoleService("console", modifier);
    GlobalThread::create(input_thread, CPU::current().log_id(), "console-input")->start();
    srv->start();
    return 0;
}
