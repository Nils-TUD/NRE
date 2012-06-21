/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <arch/Types.h>
#include <stream/OStream.h>

namespace nul {

/**
 * Holds all properties of a dataspace, which can be passed around to describe a dataspace.
 */
class DataSpaceDesc {
public:
	enum Type {
		ANONYMOUS,
		LOCKED
	};
	enum Perm {
		// note that this equals the values in a Crd
		R	= 1 << 0,
		W	= 1 << 1,
		X	= 1 << 2,
		RW	= R | W,
		RX	= R | X,
		RWX	= R | W | X,
	};

	/**
	 * Creates an empty descriptor
	 */
	explicit DataSpaceDesc() : _virt(), _origin(), _size(), _perm(), _type() {
	}
	/**
	 * Creates a descriptor from given parameters
	 *
	 * @param size the size in bytes
	 * @param type the type of memory
	 * @param perm the permissions
	 * @param phys the physical address to request (optionally)
	 * @param virt the virtual address, which can be used as a hint
	 */
	explicit DataSpaceDesc(size_t size,Type type,uint perm,uintptr_t phys = 0,uintptr_t virt = 0)
		: _virt(virt), _origin(phys), _size(size), _perm(perm), _type(type) {
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
	 * The origin of the memory
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
	 * The permissions
	 */
	uint perm() const {
		return _perm;
	}
	void perm(uint perm) {
		_perm = perm;
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
	uintptr_t _origin;
	size_t _size;
	uint _perm;
	Type _type;
};

static inline OStream &operator<<(OStream &os,const DataSpaceDesc &desc) {
	os.writef("virt=%p size=%zu origin=%p perm=%#x, type=%u",
			desc.virt(),desc.size(),desc.origin(),desc.perm(),desc.type());
	return os;
}

}
