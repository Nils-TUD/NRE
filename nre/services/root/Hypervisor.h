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

#include <kobj/Pt.h>
#include <kobj/Ports.h>
#include <cap/CapRange.h>
#include <mem/RegionManager.h>
#include <util/BitField.h>
#include <Hip.h>

/**
 * This class is used to request resources from the hypervisor Pd. Additionally, it manages all
 * I/O ports and GSIs in the sense that it keeps track what are in use by some task and which ones
 * are free.
 */
class Hypervisor {
public:
	/**
	 * Inits this module
	 */
	static void init();

	PORTAL static void portal_map(capsel_t pid);
	PORTAL static void portal_gsi(capsel_t pid);
	PORTAL static void portal_io(capsel_t pid);

	/**
	 * Maps <size> bytes at <phys> to <virt>. It assumes that both the virtual and the physical
	 * pages are available.
	 */
	static void map_mem(uintptr_t phys,uintptr_t virt,size_t size);
	/**
	 * Undos the operation of map_mem(). That is, unmaps <size> bytes at <virt>.
	 */
	static void unmap_mem(uintptr_t virt,size_t size);

	/**
	 * Maps the string at physical address <phys> to a free virtual address. The string can be
	 * at most <max_pages> long, starting at the beginning of the page of <phys>.
	 * This function is intended for Hip-module cmdlines.
	 */
	static char *map_string(uintptr_t phys,uint max_pages = 2);
	/**
	 * Undos the operation of map_string(). That is, it unmaps the pages the given string touches
	 * and free's the virtual memory that has been allocated.
	 */
	static void unmap_string(const char *str);

	/**
	 * Allocates the given gsi. This will mark it as unavailable. You can either specify a known
	 * gsi to allocate or specify <pcicfg> to determine the gsi automatically.
	 *
	 * @param gsi the gsi (will be determined automatically if pcicfg != 0)
	 * @param pcicfg the address in the PCI configuration space of the device
	 */
	static void allocate_gsi(uint &gsi,void *pcicfg = 0) {
		nre::ScopedLock<nre::UserSm> guard(&_gsi_sm);
		if(pcicfg)
			gsi = nre::Hip::get().cfg_gsi - ++_next_msi;
		if(_gsis.is_set(gsi))
			throw nre::Exception(nre::E_EXISTS,32,"GSI %u already in use",gsi);
		_gsis.set(gsi);
	}
	/**
	 * Releases <gsi>
	 *
	 * @param gsi the gsi to release
	 */
	static void release_gsi(uint gsi) {
		nre::ScopedLock<nre::UserSm> guard(&_gsi_sm);
		_gsis.clear(gsi);
	}

	/**
	 * Allocates the I/O ports <base>+<count>-1. This will mark them as unavailable.
	 *
	 * @param base the start of the port range
	 * @param count the number of ports
	 */
	static void allocate_ports(nre::Ports::port_t base,uint count) {
		nre::ScopedLock<nre::UserSm> guard(&_io_sm);
		_io.alloc_region(base,count);
	}
	/**
	 * Releases the I/O ports <base>+<count>-1.
	 *
	 * @param base the start of the port range
	 * @param count the number of ports
	 */
	static void release_ports(nre::Ports::port_t base,uint count) {
		nre::ScopedLock<nre::UserSm> guard(&_io_sm);
		_io.free(base,count);
	}

	/**
	 * Requests the idle Sc for the given cpu from NOVA
	 *
	 * @param cpu the physical cpu id
	 * @return the idle Sc cap
	 */
	static capsel_t request_idle_sc(cpu_t cpu) {
		nre::UtcbFrame uf;
		uf.accept_delegates(0,nre::Crd::OBJ_ALL);
		uf << nre::CapRange(cpu,1,nre::Crd::OBJ_ALL);
		_map_pts[nre::CPU::current().log_id()]->call(uf);
		return uf.get_delegated(0).offset();
	}

private:
	Hypervisor();

	static uchar _stack[];
	static nre::Pt *_map_pts[];
	static nre::RegionManager _io;
	static nre::BitField<nre::Hip::MAX_GSIS> _gsis;
	static nre::UserSm _io_sm;
	static nre::UserSm _gsi_sm;
	static uint _next_msi;
	static Hypervisor _init;
};
