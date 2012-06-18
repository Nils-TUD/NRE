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

namespace nul {

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

	DataSpaceDesc() : _virt(), _phys(), _size(), _perm(), _type() {
	}
	DataSpaceDesc(size_t size,Type type,uint perm,uintptr_t phys = 0,uintptr_t virt = 0)
		: _virt(virt), _phys(phys), _size(size), _perm(perm), _type(type) {
	}

	uintptr_t virt() const {
		return _virt;
	}
	void virt(uintptr_t addr) {
		_virt = addr;
	}
	uintptr_t phys() const {
		return _phys;
	}
	void phys(uintptr_t addr) {
		_phys = addr;
	}
	size_t size() const {
		return _size;
	}
	void size(size_t size) {
		_size = size;
	}
	uint perm() const {
		return _perm;
	}
	void perm(uint perm) {
		_perm = perm;
	}
	Type type() const {
		return _type;
	}

private:
	uintptr_t _virt;
	uintptr_t _phys;
	size_t _size;
	uint _perm;
	Type _type;
};

}
