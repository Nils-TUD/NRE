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
	ConsoleStream cs(_cons,0);

	// display header
	cs.writef("%*s: ",MAX_NAME_LEN,"Sc");
	for(CPU::iterator cpu = CPU::begin(); cpu != CPU::end(); ++cpu)
		cs.writef(" CPU%u ",cpu->log_id());
	cs.writef("%*s",MAX_SUMTIME_LEN + 1,"Total");
	cs.writef("\n");
	for(uint i = 0; i < Console::COLS; i++)
		cs << '-';

	// determine the total time elapsed on each CPU. this way we don't assume that exactly 1sec
	// has passed since last update and are thus less dependend on the timer-service.
	static timevalue_t cputotal[Hip::MAX_CPUS];
	for(CPU::iterator cpu = CPU::begin(); cpu != CPU::end(); ++cpu)
		cputotal[cpu->log_id()] = _sysinfo.get_total_time(cpu->log_id(),update);

	for(size_t idx = _top, c = 0; c < ROWS; ++c, ++idx) {
		SysInfo::TimeUser tu;
		if(!_sysinfo.get_timeuser(idx,tu))
			break;

		size_t namelen = 0;
		const char *name = getname(tu.name(),namelen);
		namelen = Math::min<size_t>(namelen,MAX_NAME_LEN);
		// writef doesn't support floats, so calculate it with 1000 as base and use the last decimal
		// for the first fraction-digit.
		timevalue_t permil;
		if(tu.time() == 0)
			permil = 0;
		else
			permil = (timevalue_t)(1000 / ((float)cputotal[tu.cpu()] / tu.time()));
		cs.writef("%*.*s: %*s%3Lu.%Lu%*s%*Lums\n",MAX_NAME_LEN,namelen,name,tu.cpu() * MAX_TIME_LEN,"",
				permil / 10,permil % 10,(CPU::count() - tu.cpu() - 1) * MAX_TIME_LEN,"",
				MAX_SUMTIME_LEN,tu.totaltime() / 1000);
	}
	display_footer(cs,0);
}
