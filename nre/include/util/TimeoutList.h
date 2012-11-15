/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2007-2008, Bernhard Kauer <bk@vmmon.org>
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
#include <Exception.h>
#include <Assert.h>

namespace nre {

class TimeoutListException : public Exception {
public:
    DEFINE_EXCONSTRS(TimeoutListException)
};

/**
 * Keeping track of the timeouts.
 */
template<unsigned ENTRIES, typename DATA>
class TimeoutList {
    class TimeoutEntry {
        friend class TimeoutList<ENTRIES, DATA>;
        TimeoutEntry *_next;
        TimeoutEntry *_prev;
        timevalue_t _timeout;
        DATA * data;
        bool _free;
    };

public:
    explicit TimeoutList() : _entries() {
        for(size_t i = 0; i < ENTRIES; i++) {
            _entries[i]._prev = _entries + i;
            _entries[i]._next = _entries + i;
            _entries[i].data = nullptr;
            _entries[i]._free = true;
        }
        _entries[0]._timeout = ~0ULL;
    }

    /**
     * Alloc a new timeout object.
     */
    size_t alloc(DATA *data = nullptr) {
        for(size_t i = 1; i < ENTRIES; i++) {
            if(!_entries[i]._free)
                continue;
            _entries[i].data = data;
            _entries[i]._free = false;
            return i;
        }
        throw TimeoutListException(E_CAPACITY, "No free timeout slots");
    }

    /**
     * Dealloc a timeout object.
     */
    bool dealloc(size_t nr, bool withcancel = false) {
        assert(nr >= 1 && nr <= ENTRIES - 1);
        if(_entries[nr]._free)
            return false;

        // should only be done when no no concurrent access happens ...
        if(withcancel)
            cancel(nr);
        _entries[nr]._free = true;
        _entries[nr].data = nullptr;
        return true;
    }

    /**
     * Cancel a programmed timeout.
     */
    bool cancel(size_t nr) {
        assert(nr >= 1 && nr <= ENTRIES - 1);
        TimeoutEntry *current = _entries + nr;
        if(current->_next == current)
            return false;
        bool res = _entries[0]._next != current;
        current->_next->_prev = current->_prev;
        current->_prev->_next = current->_next;
        current->_next = current->_prev = current;
        return res;
    }

    /**
     * Request a new timeout.
     */
    bool request(size_t nr, timevalue_t to) {
        assert(nr >= 1 && nr <= ENTRIES - 1);
        timevalue_t old = timeout();
        TimeoutEntry *current = _entries + nr;
        cancel(nr);

        // keep a sorted list here
        TimeoutEntry *t = _entries;
        do {
            t = t->_next;
        }
        while(t->_timeout < to);

        current->_timeout = to;
        current->_next = t;
        current->_prev = t->_prev;
        t->_prev->_next = current;
        t->_prev = current;
        return timeout() == old;
    }

    /**
     * Get the head of the queue.
     */
    size_t trigger(timevalue_t now, DATA **data = nullptr) {
        if(now >= timeout()) {
            size_t i = _entries[0]._next - _entries;
            if(data)
                *data = _entries[i].data;
            return i;
        }
        return 0;
    }

    timevalue_t timeout() {
        assert(_entries[0]._next);
        return _entries[0]._next->_timeout;
    }

private:
    TimeoutEntry _entries[ENTRIES];
};

}
