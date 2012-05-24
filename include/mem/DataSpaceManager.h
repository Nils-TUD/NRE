/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

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

	void create(DataSpace& ds) {
		Slot *slot = find_free();
		ds._origin = ds.domap();
		slot->ds = ds;
		slot->refs = 1;
	}
	uintptr_t join(capsel_t sel) {
		for(size_t i = 0; i < MAX_SLOTS; ++i) {
			if(_slots[i].refs && _slots[i].ds.sel() == sel) {
				_slots[i].refs++;
				return _slots[i].ds.origin();
			}
		}
		return 0;
	}
	bool destroy(capsel_t sel) {
		bool res = false;
		for(size_t i = 0; i < MAX_SLOTS; ++i) {
			if(_slots[i].refs && _slots[i].ds.sel() == sel) {
				if(--_slots[i].refs == 0) {
					_slots[i].ds.dounmap();
					res = true;
				}
				break;
			}
		}
		return res;
	}

private:
	Slot *find_free() const {
		for(size_t i = 0; i < MAX_SLOTS; ++i) {
			if(_slots[i].refs == 0)
				return _slots + i;
		}
		// TODO different exception
		throw Exception("No free slot");
	}

	Slot _slots[MAX_SLOTS];
};

}
