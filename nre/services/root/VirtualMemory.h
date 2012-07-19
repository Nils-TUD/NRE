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
		size = nre::Math::round_up<size_t>(size,nre::ExecEnv::PAGE_SIZE);
		if(RAM_BEGIN + phys >= RAM_END)
			return 0;
		if(RAM_BEGIN + phys + size > RAM_END)
			size = RAM_END - (RAM_BEGIN + phys);
		return RAM_BEGIN + phys;
	}
	static uintptr_t phys_to_virt(uintptr_t virt) {
		return virt + RAM_BEGIN;
	}
	static uintptr_t virt_to_phys(uintptr_t virt) {
		return virt - RAM_BEGIN;
	}

	static uintptr_t alloc(size_t size) {
		nre::ScopedLock<nre::UserSm> guard(&_sm);
		return _regs.alloc(nre::Math::round_up<size_t>(size,nre::ExecEnv::PAGE_SIZE));
	}
	static void free(uintptr_t addr,size_t size) {
		nre::ScopedLock<nre::UserSm> guard(&_sm);
		_regs.free(addr,nre::Math::round_up<size_t>(size,nre::ExecEnv::PAGE_SIZE));
	}

	static const nre::RegionManager &regions() {
		return _regs;
	}

private:
	VirtualMemory();

	static nre::RegionManager _regs;
	static nre::UserSm _sm;
	static VirtualMemory _init;
};
