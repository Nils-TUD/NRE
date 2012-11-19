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

#include <util/SList.h>

namespace nre {

/**
 * A singly linked list that maintains an order, determined by a specified comparison function.
 */
template<class T>
class SortedSList {
public:
    typedef typename SList<T>::iterator iterator;
    typedef bool (*cmp_func)(const T &a, const T &b);

    /**
     * Constructor. Creates an empty list.
     *
     * @param isless the comparison function
     */
    explicit SortedSList(cmp_func isless) : _isless(isless), _list() {
    }

    /**
     * @return the number of items in the list
     */
    size_t length() const {
        return _list.length();
    }

    /**
     * @return beginning of list
     */
    iterator begin() const {
        return _list.begin();
    }
    /**
     * @return end of list
     */
    iterator end() const {
        return _list.end();
    }
    /**
     * @return tail of the list, i.e. the last valid item
     */
    iterator tail() const {
        return _list.tail();
    }

    /**
     * Inserts the given item into the list at the corresponding position. This works in
     * linear time.
     *
     * @param e the list item
     * @return the position where it has been inserted
     */
    iterator insert(T *e) {
        T *p = nullptr;
        iterator it;
        for(it = begin(); it != end() && _isless(*it, *e); ++it)
            p = &*it;
        return _list.insert(p, e);
    }
    /**
     * Removes the given item from the list. This works in linear time.
     * Does NOT expect that the item is in the list!
     *
     * @param e the list item
     * @return true if the item has been found and removed
     */
    bool remove(T *e) {
        return _list.remove(e);
    }

private:
    cmp_func _isless;
    SList<T> _list;
};

}
