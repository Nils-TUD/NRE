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

template<class DS>
class DataSpaceManager;

template<class DS>
static inline OStream &operator<<(OStream &os,const DataSpaceManager<DS> &mng);

/**
 * The DataSpaceManager is responsible for keeping track of the number of references to a dataspace.
 * That is, you can create dataspaces, join dataspaces and release them again and this class will
 * make sure, that a dataspace is destroyed only when there are no references anymore.
 */
template<class DS>
class DataSpaceManager {
	template<class DS2>
	friend OStream &operator<<(OStream &os,const DataSpaceManager<DS2> &mng);

	enum {
		MAX_SLOTS	= 128
	};

	struct Slot {
		DS *ds;
		unsigned refs;
	};

public:
	explicit DataSpaceManager() : _slots() {
	}
	virtual ~DataSpaceManager() {
	}

	/**
	 * Creates a new dataspace by given description and returns a reference to it (the properties
	 * of the dataspace are adjusted during the creation)
	 *
	 * @param desc the desciption
	 * @return the created dataspace
	 * @throws DataSpaceException if there are no free slots anymore
	 */
	const DS &create(const DataSpaceDesc& desc) {
		Slot *slot = 0;
		slot = find_free();
		slot->ds = new DS(desc);
		slot->refs = 1;
		return *slot->ds;
	}

	/**
	 * Joins the dataspace identified by the given selector. The descriptor is more or less ignored
	 * since the properties are fetched from the already created dataspace. But might be used for
	 * verification.
	 *
	 * @param desc the descriptor
	 * @param sel the selector
	 * @return the dataspace
	 * @throws DataSpaceException if there are no free slots anymore
	 */
	const DS &join(const DataSpaceDesc &desc,capsel_t sel) {
		Slot *slot = find(sel);
		if(!slot) {
			slot = find_free();
			slot->ds = new DS(desc,sel);
			slot->refs = 1;
		}
		else
			slot->refs++;
		return *slot->ds;
	}

	/**
	 * Releases the dataspace identified by the given selector, i.e. the number of references are
	 * decreased. If it reaches zero, the dataspace is destroyed.
	 *
	 * @param desc the descriptor. Is updated from the found dataspace
	 * @param sel the selector
	 * @throws DataSpaceException if the dataspace was not found
	 */
	void release(DataSpaceDesc &desc,capsel_t sel) {
		for(size_t i = 0; i < MAX_SLOTS; ++i) {
			if(_slots[i].refs && _slots[i].ds->unmapsel() == sel) {
				if(--_slots[i].refs == 0) {
					desc = _slots[i].ds->desc();
					delete _slots[i].ds;
					_slots[i].ds = 0;
				}
				return;
			}
		}
		throw DataSpaceException(E_NOT_FOUND);
	}

private:
	Slot *find(capsel_t sel) {
		for(size_t i = 0; i < MAX_SLOTS; ++i) {
			if(_slots[i].refs && _slots[i].ds->sel() == sel)
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

template<class DS>
static inline OStream &operator<<(OStream &os,const DataSpaceManager<DS> &mng) {
	os << "DataSpaces:\n";
	for(size_t i = 0; i < DataSpaceManager<DS>::MAX_SLOTS; ++i) {
		if(mng._slots[i].refs)
			os << "\tSlot " << i << ":" << *(mng._slots[i].ds) << " (" << mng._slots[i].refs << " refs)\n";
	}
	return os;
}

}
