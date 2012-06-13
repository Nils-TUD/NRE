/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <mem/DataSpace.h>
#include <Exception.h>

namespace nul {

class DataSpaceManager {
	enum {
		MAX_SLOTS	= 64
	};

	struct Slot {
		DataSpace ds;
		unsigned refs;
	};

public:
	DataSpaceManager() : _slots() {
	}

	bool create(DataSpace& ds) {
		Slot *slot = 0;
		if(ds.sel() != ObjCap::INVALID) {
			slot = find(ds);
			if(!slot)
				throw DataSpaceException(E_NOT_FOUND);
			ds = slot->ds;
			slot->refs++;
			return false;
		}

		// TODO phys
		slot = find_free();
		ds.map();
		slot->ds = ds;
		slot->refs = 1;
		return true;
	}
	DataSpace *destroy(capsel_t sel,bool &destroyable) {
		for(size_t i = 0; i < MAX_SLOTS; ++i) {
			if(_slots[i].refs && _slots[i].ds.unmapsel() == sel) {
				if(--_slots[i].refs == 0)
					destroyable = true;
				return &_slots[i].ds;
			}
		}
		throw DataSpaceException(E_NOT_FOUND);
	}

private:
	Slot *find(const DataSpace& ds) {
		for(size_t i = 0; i < MAX_SLOTS; ++i) {
			if(_slots[i].refs && _slots[i].ds.sel() == ds.sel())
				return _slots + i;
		}
		return 0;
	}
	Slot *find_free() {
		for(size_t i = 0; i < MAX_SLOTS; ++i) {
			if(_slots[i].refs == 0)
				return _slots + i;
		}
		throw DataSpaceException(E_CAPACITY);
	}

	Slot _slots[MAX_SLOTS];
};

}
