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

#include "SysInfoPage.h"

using namespace nre;

void PdInfoPage::refresh_console(bool) {
    ScopedLock<UserSm> guard(&_sm);
    _cons.clear(0);
    ConsoleStream cs(_cons, 0);

    // display header
    size_t memtotal, memfree;
    _sysinfo.get_mem(memtotal, memfree);
    cs << fmt("Pd", MAX_NAME_LEN) << ": " << fmt("VirtMem", 24) << fmt("PhysMem", 24)
       << fmt("Threads", 8) << "\n";
    for(uint i = 0; i < Console::COLS; i++)
        cs << '-';

    size_t totalthreads = 0;
    size_t totalphys = 0;
    size_t totalvirt = 0;
    for(size_t idx = 0, c = 0; c < ROWS; ++c, ++idx) {
        SysInfo::Child c;
        if(!_sysinfo.get_child(idx, c))
            break;

        if(idx >= _top) {
            size_t namelen = 0;
            const char *name = getname(c.cmdline(), namelen);
            cs << fmt(name, MAX_NAME_LEN, namelen) << ": "
               << fmt(c.virt_mem() / 1024, 20) << " KiB"
               << fmt(c.phys_mem() / 1024, 20) << " KiB"
               << fmt(c.threads(), 8) << "\n";
        }
        totalvirt += c.virt_mem();
        totalphys += c.phys_mem();
        totalthreads += c.threads();
    }

    // display footer
    for(uint i = 0; i < Console::COLS; i++)
        cs << '-';
    cs << fmt("Total", MAX_NAME_LEN) << ": "
       << fmt(totalvirt / 1024, 20) << " KiB"
       << fmt(totalphys / 1024, 8) << " of " << fmt(memtotal / 1024, 8) << " KiB"
       << fmt(totalthreads, 8) << "\n";
    display_footer(cs, 1);
}
