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
		RootDataSpace(const nre::DataSpaceDesc &desc);
		RootDataSpace(const nre::DataSpaceDesc &,capsel_t);
		~RootDataSpace();

		capsel_t sel() const {
			return _sel;
		}
		capsel_t unmapsel() const {
			return _unmapsel;
		}
		const nre::DataSpaceDesc &desc() const {
			return _desc;
		}

		// we have to provide custom new and delete operators since we can't use dynamic memory for
		// building dynamic memory :)
		static void *operator new(size_t size) throw();
		static void operator delete(void *ptr) throw();

	private:
		static void revoke_mem(uintptr_t addr,size_t size,bool self = false);

		nre::DataSpaceDesc _desc;
		capsel_t _sel;
		capsel_t _unmapsel;
		static RootDataSpace _slots[MAX_SLOTS];
	};

	static void add(uintptr_t addr,size_t size);
	static void remove(uintptr_t addr,size_t size);
	static void map_all();

	static const nre::RegionManager &regions() {
		return _mem;
	}

	PORTAL static void portal_dataspace(capsel_t);

private:
	static bool can_map(uintptr_t phys,size_t size,uint &flags);

	PhysicalMemory();

	static nre::RegionManager _mem;
	static nre::DataSpaceManager<RootDataSpace> _dsmng;
};

static inline nre::OStream &operator<<(nre::OStream &os,const PhysicalMemory::RootDataSpace &ds) {
	os.writef("RootDataSpace[sel=%#x, umsel=%#x]: ",ds.sel(),ds.unmapsel());
	os << ds.desc();
	return os;
}
