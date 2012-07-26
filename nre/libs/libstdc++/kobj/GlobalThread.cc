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

#include <arch/Startup.h>
#include <kobj/GlobalThread.h>
#include <cap/CapSelSpace.h>

namespace nre {

GlobalThread GlobalThread::_cur INIT_PRIO_GEC (
	_startup_info.utcb,CapSelSpace::INIT_EC,_startup_info.cpu,&Pd::_cur,_startup_info.stack
);

}
