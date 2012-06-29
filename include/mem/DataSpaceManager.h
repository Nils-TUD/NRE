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
#include <kobj/UserSm.h>
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
		MAX_SLOTS	= 256
	};

	struct Slot {
		DS *ds;
		unsigned refs;
	};

public:
	explicit DataSpaceManager() : _sm(), _slots() {
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
		ScopedLock<UserSm> guard(&_sm);
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
		ScopedLock<UserSm> guard(&_sm);
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
	 * Swaps the virt-property of the two dataspaces specified by ds1 and ds2
	 *
	 * @param ds1 the unmap-selector for the first dataspace
	 * @param ds2 the unmap-selector for the second dataspace
	 */
	void swap(capsel_t ds1,capsel_t ds2) {
		ScopedLock<UserSm> guard(&_sm);
		Slot *s1 = find_unmap(ds1);
		Slot *s2 = find_unmap(ds2);
		if(!s1 || !s2)
			throw DataSpaceException(E_NOT_FOUND);

		uintptr_t tmp = s1->ds->_desc.virt();
		s1->ds->_desc.virt(s2->ds->_desc.virt());
		s2->ds->_desc.virt(tmp);
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
		ScopedLock<UserSm> guard(&_sm);
		Slot *s = find_unmap(sel);
		if(!s)
			throw DataSpaceException(E_NOT_FOUND);
		if(--s->refs == 0) {
			desc = s->ds->desc();
			delete s->ds;
			s->ds = 0;
		}
	}

private:
	Slot *find(capsel_t sel) {
		for(size_t i = 0; i < MAX_SLOTS; ++i) {
			if(_slots[i].refs && _slots[i].ds->sel() == sel)
				return _slots + i;
		}
		return 0;
	}
	Slot *find_unmap(capsel_t sel) {
		for(size_t i = 0; i < MAX_SLOTS; ++i) {
			if(_slots[i].refs && _slots[i].ds->unmapsel() == sel)
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

	UserSm _sm;
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
