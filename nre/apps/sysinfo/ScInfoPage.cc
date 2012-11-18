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

void ScInfoPage::refresh_console(bool update) {
    ScopedLock<UserSm> guard(&_sm);
    _cons.clear(0);
    ConsoleStream cs(_cons, 0);

    // display header
    cs << fmt("Sc", MAX_NAME_LEN) << ": ";
    for(auto cpu = CPU::begin(); cpu != CPU::end(); ++cpu)
        cs << " CPU" << cpu->log_id() << " ";
    cs << fmt("Total", MAX_SUMTIME_LEN + 1) << "\n";
    for(uint i = 0; i < Console::COLS; i++)
        cs << '-';

    // determine the total time elapsed on each CPU. this way we don't assume that exactly 1sec
    // has passed since last update and are thus less dependend on the timer-service.
    static timevalue_t cputotal[Hip::MAX_CPUS];
    for(auto cpu = CPU::begin(); cpu != CPU::end(); ++cpu)
        cputotal[cpu->log_id()] = _sysinfo.get_total_time(cpu->log_id(), update);

    for(size_t idx = _top, c = 0; c < ROWS; ++c, ++idx) {
        SysInfo::TimeUser tu;
        if(!_sysinfo.get_timeuser(idx, tu))
            break;

        size_t namelen = 0;
        const char *name = getname(tu.name(), namelen);
        namelen = Math::min<size_t>(namelen, MAX_NAME_LEN);
        double permil;
        if(tu.time() == 0)
            permil = 0;
        else
            permil = 100. / (static_cast<double>(cputotal[tu.cpu()]) / tu.time());
        cs << fmt(name, MAX_NAME_LEN, namelen) << ": " << fmt("", tu.cpu() * MAX_TIME_LEN)
           << fmt(permil, 5, 1) << fmt("", (CPU::count() - tu.cpu() - 1) * MAX_TIME_LEN)
           << fmt(tu.totaltime() / 1000, MAX_SUMTIME_LEN) << "ms\n";
    }
    display_footer(cs, 0);
}
