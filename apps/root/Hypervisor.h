/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <kobj/Pt.h>
#include <kobj/Ports.h>
#include <cap/CapRange.h>
#include <mem/RegionManager.h>
#include <BitField.h>
#include <Hip.h>

class Hypervisor {
public:
	PORTAL static void portal_mem(capsel_t pid);
	PORTAL static void portal_gsi(capsel_t pid);
	PORTAL static void portal_io(capsel_t pid);

	static void map_mem(uintptr_t phys,uintptr_t virt,size_t size);
	static char *map_string(uintptr_t phys,uint max_pages = 2);

	static void revoke_io(nul::Ports::port_t base,uint count);

	static void allocate_gsi(uint gsi) {
		nul::ScopedLock<nul::UserSm> guard(&_gsi_sm);
		if(_gsis.is_set(gsi))
			throw nul::Exception(nul::E_EXISTS);
		_gsis.set(gsi);
	}
	static void release_gsi(uint gsi) {
		nul::ScopedLock<nul::UserSm> guard(&_gsi_sm);
		_gsis.clear(gsi);
	}

	static void allocate_ports(nul::Ports::port_t base,uint count) {
		nul::ScopedLock<nul::UserSm> guard(&_io_sm);
		_io.alloc(base,count);
	}
	static void release_ports(nul::Ports::port_t base,uint count) {
		nul::ScopedLock<nul::UserSm> guard(&_io_sm);
		_io.free(base,count);
	}

private:
	Hypervisor();

	static uchar _stack[];
	static nul::Pt *_mem_pt;
	static nul::RegionManager _io;
	static nul::BitField<nul::Hip::MAX_GSIS> _gsis;
	static nul::UserSm _io_sm;
	static nul::UserSm _gsi_sm;
	static Hypervisor _init;
};
