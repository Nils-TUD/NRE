/*
 * TODO comment me
 *
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NUL.
 *
 * NUL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NUL is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#include <arch/Startup.h>
#include <kobj/Pd.h>
#include <kobj/GlobalEc.h>

using namespace nul;

void _setup() {
	static Pd initpd(Hip::get().cfg_exc + 0,true);
	static GlobalEc initec(
		_startup_info.utcb,Hip::get().cfg_exc + 1,_startup_info.cpu,&initpd
	);
}
