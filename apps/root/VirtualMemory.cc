/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include "VirtualMemory.h"

using namespace nul;

extern void *edata;

RegionManager VirtualMemory::_regs;
UserSm *VirtualMemory::_sm;

void VirtualMemory::init() {
	uintptr_t begin = Math::round_up<uintptr_t>(reinterpret_cast<uintptr_t>(&edata),ExecEnv::PAGE_SIZE);
	_regs.free(begin,RAM_BEGIN - begin);
	_sm = new UserSm();
}

