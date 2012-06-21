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
#include <kobj/UserSm.h>
#include <mem/RegionManager.h>
#include <mem/DataSpaceManager.h>

class PhysicalMemory {
public:
	class RootDataSpace;
	friend class RootDataSpace;

	class RootDataSpace {
		enum {
			MAX_SLOTS = 256
		};

	public:
		RootDataSpace() : _desc(), _sel(), _unmapsel() {
		}
		RootDataSpace(const nul::DataSpaceDesc &desc);
		RootDataSpace(const nul::DataSpaceDesc &,capsel_t);
		~RootDataSpace();

		capsel_t sel() const {
			return _sel;
		}
		capsel_t unmapsel() const {
			return _unmapsel;
		}
		const nul::DataSpaceDesc &desc() const {
			return _desc;
		}

		// we have to provide custom new and delete operators since we can't use dynamic memory for
		// building dynamic memory :)
		static void *operator new(size_t size) throw();
		static void operator delete(void *ptr) throw();

	private:
		static void revoke_mem(uintptr_t addr,size_t size);

		nul::DataSpaceDesc _desc;
		capsel_t _sel;
		capsel_t _unmapsel;
		static RootDataSpace _slots[MAX_SLOTS];
	};

	static void init();

	static void add(uintptr_t addr,size_t size);
	static void remove(uintptr_t addr,size_t size);
	static void map_all();

	static const nul::RegionManager &regions() {
		return _mem;
	}

	PORTAL static void portal_map(capsel_t);
	PORTAL static void portal_unmap(capsel_t);

private:
	static bool can_map(uintptr_t phys,size_t size,uint &flags);

	PhysicalMemory();

	static nul::UserSm *_sm;
	static nul::RegionManager _mem;
	static nul::DataSpaceManager<RootDataSpace> _dsmng;
};

static inline nul::OStream &operator<<(nul::OStream &os,const PhysicalMemory::RootDataSpace &ds) {
	os.writef("RootDataSpace[sel=%#x, umsel=%#x]: ",ds.sel(),ds.unmapsel());
	os << ds.desc();
	return os;
}
