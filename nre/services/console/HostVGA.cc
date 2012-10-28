/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2009, Bernhard Kauer <bk@vmmon.org>
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

#include "HostVGA.h"

using namespace nre;

void HostVGA::set_regs(const nre::Console::Register &regs) {
    if(regs.cursor_style != _last.cursor_style) {
        write(CURSOR_HI, regs.cursor_style >> 8);
        write(CURSOR_LO, regs.cursor_style);
    }
    if(regs.cursor_pos != _last.cursor_pos) {
        uint16_t cursor_offset = regs.cursor_pos - (Console::TEXT_OFF >> 1);
        write(CURSOR_LOC_LO, cursor_offset);
        write(CURSOR_LOC_HI, cursor_offset >> 8);
    }
    if(regs.offset != _last.offset) {
        uintptr_t offset = regs.offset - (Console::TEXT_OFF >> 1);
        write(START_ADDR_HI, offset >> 8);
        write(START_ADDR_LO, offset);
    }
    _last = regs;
}
