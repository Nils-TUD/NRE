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

#include "VirtualMemory.h"

using namespace nre;

extern void *edata;

RegionManager VirtualMemory::_regs INIT_PRIO_VMEM;
UserSm VirtualMemory::_sm INIT_PRIO_VMEM;
VirtualMemory VirtualMemory::_init INIT_PRIO_VMEM;

VirtualMemory::VirtualMemory() {
	uintptr_t begin = Math::round_up<uintptr_t>(reinterpret_cast<uintptr_t>(&edata),ExecEnv::PAGE_SIZE);
	_regs.free(begin,RAM_BEGIN - begin);
}

