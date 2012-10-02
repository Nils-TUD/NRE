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

#include <arch/Types.h>
#include <stream/OStream.h>

namespace nre {

/**
 * Holds all properties of a dataspace, which can be passed around to describe a dataspace.
 */
class DataSpaceDesc {
public:
	enum Type {
		// the mapping is not backed by any file
		ANONYMOUS,
		// the mapping is not backed by memory at all. i.e. it's just a piece of virtual memory
		// that has been reserved.
		VIRTUAL,
		// will never be swapped out
		LOCKED
	};
	enum Flags {
		// note that this equals the values in a Crd
		R			= 1 << 0,
		W			= 1 << 1,
		X			= 1 << 2,
		RW			= R | W,
		RX			= R | X,
		RWX			= R | W | X,
		BIGPAGES	= 1 << 3,	// align to 4M in virtual memory to allow 4M pages
	};

	/**
	 * Creates an empty descriptor
	 */
	explicit DataSpaceDesc() : _virt(), _phys(), _origin(), _size(), _flags(), _type() {
	}
	/**
	 * Creates a descriptor from given parameters
	 *
	 * @param size the size in bytes
	 * @param type the type of memory
	 * @param flags the flags
	 * @param phys the physical address to request (optionally)
	 * @param virt the virtual address (ignored)
	 * @param origin the origin of the memory (only used by the dataspace infrastructure)
	 */
	explicit DataSpaceDesc(size_t size,Type type,uint flags,uintptr_t phys = 0,uintptr_t virt = 0,
			uintptr_t origin = 0)
		: _virt(virt), _phys(phys), _origin(origin), _size(size), _flags(flags), _type(type) {
	}

	/**
	 * The virtual address
	 */
	uintptr_t virt() const {
		return _virt;
	}
	void virt(uintptr_t addr) {
		_virt = addr;
	}

	/**
	 * The physical address
	 */
	uintptr_t phys() const {
		return _phys;
	}
	void phys(uintptr_t addr) {
		_phys = addr;
	}

	/**
	 * The origin of the memory (only used by the dataspace infrastructure)
	 */
	uintptr_t origin() const {
		return _origin;
	}
	void origin(uintptr_t addr) {
		_origin = addr;
	}

	/**
	 * The size in bytes
	 */
	size_t size() const {
		return _size;
	}
	void size(size_t size) {
		_size = size;
	}

	/**
	 * The flags
	 */
	uint flags() const {
		return _flags;
	}
	void flags(uint flags) {
		_flags = flags;
	}

	/**
	 * @return the type of memory
	 */
	Type type() const {
		return _type;
	}
	void type(Type type) {
		_type = type;
	}

private:
	uintptr_t _virt;
	uintptr_t _phys;
	uintptr_t _origin;
	size_t _size;
	uint _flags;
	Type _type;
};

static inline OStream &operator<<(OStream &os,const DataSpaceDesc &desc) {
	os.writef("virt=%p phys=%p size=%zu org=%p flags=%#x",
			desc.virt(),desc.phys(),desc.size(),desc.origin(),desc.flags());
	return os;
}

}
