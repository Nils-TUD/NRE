/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <mem/RegionManager.h>
#include <kobj/UserSm.h>
#include <util/ScopedLock.h>

class VirtualMemory {
	enum {
		RAM_BEGIN		= ARCH_REGIONS_END,
		RAM_END			= ARCH_KERNEL_START
	};

public:
	static uintptr_t alloc_ram(uintptr_t phys,size_t &size) {
		size = nul::Math::round_up<size_t>(size,nul::ExecEnv::PAGE_SIZE);
		if(RAM_BEGIN + phys >= RAM_END)
			return 0;
		if(RAM_BEGIN + phys + size > RAM_END)
			size = RAM_END - size;
		return RAM_BEGIN + phys;
	}
	static uintptr_t phys_to_virt(uintptr_t virt) {
		return virt + RAM_BEGIN;
	}
	static uintptr_t virt_to_phys(uintptr_t virt) {
		return virt - RAM_BEGIN;
	}

	static uintptr_t alloc(size_t size) {
		nul::ScopedLock<nul::UserSm> guard(&_sm);
		return _regs.alloc(nul::Math::round_up<size_t>(size,nul::ExecEnv::PAGE_SIZE));
	}
	static void free(uintptr_t addr,size_t size) {
		nul::ScopedLock<nul::UserSm> guard(&_sm);
		_regs.free(addr,nul::Math::round_up<size_t>(size,nul::ExecEnv::PAGE_SIZE));
	}

	static const nul::RegionManager &regions() {
		return _regs;
	}

private:
	VirtualMemory();

	static nul::RegionManager _regs;
	static nul::UserSm _sm;
	static VirtualMemory _init;
};
