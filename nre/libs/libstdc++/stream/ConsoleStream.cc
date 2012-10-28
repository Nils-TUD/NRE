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

#include <stream/ConsoleStream.h>

using namespace nre;

char ConsoleStream::read() {
    char c;
    while(1) {
        Console::ReceivePacket pk = _sess.receive();
        c = pk.character;
        if(c != '\0' && (~pk.flags & Keyboard::RELEASE))
            break;
    }
    if(_echo)
        write(c);
    return c;
}

void ConsoleStream::put(ushort value, ushort *base, uint &pos) {
    bool visible = false;
    switch(value & 0xff) {
        // ignore '\0'
        case '\0':
            return;
        // backspace
        case 8:
            if(pos)
                pos--;
            break;
        case '\n':
            pos += Console::COLS - (pos % Console::COLS);
            break;
        case '\r':
            pos -= pos % Console::COLS;
            break;
        case '\t':
            pos += Console::TAB_WIDTH - (pos % Console::TAB_WIDTH);
            break;
        default:
            visible = true;
            break;
    }

    // scroll?
    if(pos >= Console::COLS * Console::ROWS) {
        memmove(base, base + Console::COLS, (Console::ROWS - 1) * Console::COLS * 2);
        memset(base + (Console::ROWS - 1) * Console::COLS, 0, Console::COLS * 2);
        pos = Console::COLS * (Console::ROWS - 1);
    }
    if(visible)
        base[pos++] = value;
}
