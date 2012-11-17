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

#include <mem/DataSpace.h>
#include <kobj/UserSm.h>
#include <util/Treap.h>
#include <Exception.h>

namespace nre {

template<class DS>
class DataSpaceManager;

template<class DS>
static OStream &operator<<(OStream &os, const DataSpaceManager<DS> &mng);

/**
 * The DataSpaceManager is responsible for keeping track of the number of references to a dataspace.
 * That is, you can create dataspaces, join dataspaces and release them again and this class will
 * make sure, that a dataspace is destroyed only when there are no references anymore.
 */
template<class DS>
class DataSpaceManager {
    template<class DS2>
    friend OStream & operator<<(OStream &os, const DataSpaceManager<DS2> &mng);

    static const size_t MAX_SLOTS   = 512;

    /**
     * We use a treap here and use the unmap selector as key. this way, we can quickly find a ds
     * by the unmap selector. unfortunatly, we have to be able to find a ds by its selector as
     * well. we do that by linear searching. but we need the first one more often, therefore
     * its better to do it this way, I think.
     * TODO an idea would be to define that both selectors are always directly together.
     * That is, sel=X, unmapsel=X+1. this way we could use _tree.find() here as well. but I'm not
     * sure if we should do that, because it should be transparent for the user and I think its
     * difficult to achieve that.
     */
    struct Slot : public TreapNode<capsel_t> {
        Slot() : TreapNode<capsel_t>(0), ds(), refs(), next() {
        }
        DS *ds;
        unsigned refs;
        Slot *next;
    };

public:
    explicit DataSpaceManager() : _sm(), _tree(), _free(nullptr), _slots() {
        for(size_t i = 0; i < MAX_SLOTS; ++i) {
            _slots[i].next = _free;
            _free = _slots + i;
        }
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
        Slot *slot = find_free();
        slot->ds = new DS(desc);
        slot->refs = 1;
        slot->key(slot->ds->unmapsel());
        _tree.insert(slot);
        return *slot->ds;
    }

    /**
     * Joins the dataspace identified by the given selector.
     *
     * @param sel the selector
     * @return the dataspace
     * @throws DataSpaceException if there are no free slots anymore
     */
    const DS &join(capsel_t sel) {
        ScopedLock<UserSm> guard(&_sm);
        Slot *slot = find(sel);
        if(!slot) {
            slot = find_free();
            slot->ds = new DS(sel);
            slot->refs = 1;
            slot->key(slot->ds->unmapsel());
            _tree.insert(slot);
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
    void swap(capsel_t ds1, capsel_t ds2) {
        ScopedLock<UserSm> guard(&_sm);
        Slot *s1 = find_unmap(ds1);
        Slot *s2 = find_unmap(ds2);
        if(!s1 || !s2) {
            VTHROW(DataSpaceException, E_NOT_FOUND,
                   "DataSpace " << (s1 ? ds1 : ds2) << " does not exist");
        }

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
    void release(DataSpaceDesc &desc, capsel_t sel) {
        ScopedLock<UserSm> guard(&_sm);
        Slot *s = find_unmap(sel);
        if(!s)
            VTHROW(DataSpaceException, E_NOT_FOUND, "DataSpace " << sel << " does not exist");
        if(--s->refs == 0) {
            desc = s->ds->desc();
            delete s->ds;
            _tree.remove(s);
            s->ds = nullptr;
            s->next = _free;
            _free = s;
        }
    }

private:
    Slot *find(capsel_t sel) {
        for(size_t i = 0; i < MAX_SLOTS; ++i) {
            if(_slots[i].ds && _slots[i].ds->sel() == sel)
                return _slots + i;
        }
        return nullptr;
    }
    Slot *find_unmap(capsel_t sel) {
        return _tree.find(sel);
    }
    Slot *find_free() {
        if(!_free)
            throw DataSpaceException(E_CAPACITY, "No free dataspace slots");
        Slot *s = _free;
        _free = _free->next;
        return s;
    }

    UserSm _sm;
    Treap<Slot> _tree;
    Slot *_free;
    Slot _slots[MAX_SLOTS];
};

template<class DS>
static inline OStream &operator<<(OStream &os, const DataSpaceManager<DS> &mng) {
    os << "DataSpaces:\n";
    for(size_t i = 0; i < DataSpaceManager<DS>::MAX_SLOTS; ++i) {
        if(mng._slots[i].refs)
            os << "\tSlot " << i << ":" << *(mng._slots[i].ds) << " (" << mng._slots[i].refs <<
            " refs)\n";
    }
    return os;
}

}
