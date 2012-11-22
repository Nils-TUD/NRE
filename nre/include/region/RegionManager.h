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
#include <stream/OStringStream.h>
#include <collection/DList.h>
#include <util/Math.h>
#include <util/Bytes.h>
#include <Exception.h>

namespace nre {

class RegionManagerException : public Exception {
public:
    explicit RegionManagerException(ErrorCode code = E_FAILURE, const String &msg = String()) throw()
        : Exception(code, msg) {
    }
};

struct Region : public DListItem {
    uintptr_t addr;
    size_t size;
};

class PortManager;

/**
 * This class provides routines to manage a resource in a region-based fashion. For example, you
 * can manage memory with it by adding some available memory to it (by using free()) and allocating
 * it chunk by chunk via alloc() later. The class will keep track of what parts are allocated and
 * what not.
 */
template<class Reg = Region>
class RegionManager {
    friend class PortManager;
public:
    typedef typename DList<Reg>::iterator iterator;
    typedef typename DList<Reg>::const_iterator const_iterator;

    /**
     * Creates an empty region list
     */
    explicit RegionManager() : _regs() {
    }
    /**
     * Destroys all region-objects
     */
    virtual ~RegionManager() {
        for(iterator it = _regs.begin(); it != _regs.end(); ) {
            iterator old = it++;
            delete &*old;
        }
    }

    /**
     * @return the beginning of all regions
     */
    const_iterator begin() const {
        return _regs.cbegin();
    }
    /**
     * @return the end of all regions
     */
    const_iterator end() const {
        return _regs.cend();
    }

    /**
     * @return the total number of units in the region list
     */
    size_t total_count() const {
        size_t s = 0;
        for(const_iterator it = begin(); it != end(); ++it)
            s += it->size;
        return s;
    }

    /**
     * Allocates the range <start> .. <count>-1 completely from the region list.
     *
     * @param start the start address
     * @param count the number of units
     * @param free_required if true, an exception is thrown if the area is not completely free
     * @return true the total units that has been removed
     * @throws RegionManagerException if free_required is true and it isn't free
     */
    size_t alloc_at(uintptr_t start, size_t count, bool free_required = false) {
        size_t total = 0;
        for(auto it = _regs.begin(); it != _regs.end(); ++it) {
            if(nre::Math::overlapped(start, count, it->addr, it->size)) {
                // since adjacent regions are merged, it is sufficient to check whether the
                // desired range is inside this region. if not, there is something missing, which
                // means that it is already allocated.
                if(free_required && !(start >= it->addr && start + count <= it->addr + it->size)) {
                    VTHROW(RegionManagerException, E_EXISTS,
                           fmt(start, "p") << " .. " << fmt(start + count, "p") << " not free");
                }
                total += remove_from(&*it, start, count);
                // we assume that there are no duplicates, thus we can stop here
                if(free_required)
                    break;
                it = _regs.begin();
            }
        }
        return total;
    }

    /**
     * Allocates <count> units from the region list, aligned to <align>. That is, it searches for
     * a region that contains at least <count> units (aligned to <align>) and takes them from it.
     *
     * @param count the number of units to allocate
     * @param align the alignment (in units)
     * @return the address
     * @throws RegionManagerException if there is no region with enough units
     */
    uintptr_t alloc(size_t count, size_t align = 1) {
        Reg *r = get(count, align);
        if(!r) {
            VTHROW(RegionManagerException, E_CAPACITY,
                   "Unable to allocate " << count << " units aligned to " << align);
        }
        uintptr_t org = r->addr;
        uintptr_t start = (r->addr + align - 1) & ~(align - 1);
        count += start - r->addr;
        r->addr += count;
        r->size -= count;
        // if there is space left before the start due to aligning, free it
        if(start > org) {
            Reg *nr = r;
            // do we need a new region?
            if(nr->size > 0) {
                nr = new Reg;
                _regs.append(nr);
            }
            nr->addr = org;
            nr->size = start - org;
        }
        if(r->size == 0)
            remove_reg(r);
        return start;
    }

    /**
     * Frees the range <start> .. <count>-1. Assumes that this range is not present in the current
     * region list!
     *
     * @param start the start address
     * @param count the number of units
     */
    void free(uintptr_t start, size_t count) {
        Reg *p = nullptr, *n = nullptr;
        for(iterator it = _regs.begin(); it != _regs.end(); ++it) {
            if(it->addr + it->size == start)
                p = &*it;
            else if(it->addr == start + count)
                n = &*it;
            if(n && p)
                break;
        }

        if(n && p) {
            p->size += count + n->size;
            remove_reg(n);
        }
        else if(n) {
            n->addr -= count;
            n->size += count;
        }
        else if(p)
            p->size += count;
        else {
            Reg *f = new Reg;
            f->addr = start;
            f->size = count;
            _regs.append(f);
        }
    }

private:
    RegionManager(const RegionManager&);
    RegionManager& operator=(const RegionManager&);

protected:
    Reg *get(size_t count, size_t align) {
        for(auto it = _regs.begin(); it != _regs.end(); ++it) {
            if(it->size >= count) {
                uintptr_t start = (it->addr + align - 1) & ~(align - 1);
                if(it->size - (start - it->addr) >= count)
                    return &*it;
            }
        }
        return nullptr;
    }

    void remove_reg(Reg *r) {
        _regs.remove(r);
        delete r;
    }
    size_t remove_from(Reg *r, uintptr_t start, size_t count) {
        size_t res = count;
        // complete region should be removed?
        if(start <= r->addr && start + count >= r->addr + r->size) {
            res = r->size;
            remove_reg(r);
        }
        // at the beginning?
        else if(start <= r->addr) {
            r->size -= (start + count) - r->addr;
            r->addr = start + count;
        }
        // at the end?
        else if(start + count >= r->addr + r->size)
            r->size = start - r->addr;
        // in the middle
        else {
            size_t oldsize = r->size;
            r->size = start - r->addr;
            Reg *nr = new Reg;
            nr->addr = start + count;
            nr->size = (r->addr + oldsize) - nr->addr;
            _regs.append(nr);
        }
        return res;
    }

    DList<Reg> _regs;
};

template<class Reg>
static inline OStream &operator<<(OStream &os, const RegionManager<Reg> &rm) {
    for(auto it = rm.begin(); it != rm.end(); ++it) {
        os << "\t" << fmt(it->addr, "#x") << " .. " << fmt(it->addr + it->size, "#x")
           << " (" << fmt(it->size, "#x") << ")\n";
    }
    return os;
}

}
