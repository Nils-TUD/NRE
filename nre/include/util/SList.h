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
#include <Assert.h>

namespace nre {

template<class T>
class SList;
template<class T>
class SListIterator;

/**
 * A listitem for the singly linked list. It is intended that you inherit from this class to add
 * data to the item.
 */
class SListItem {
    template<class T>
    friend class SList;
    template<class T>
    friend class SListIterator;

public:
    /**
     * Constructor
     */
    explicit SListItem() : _next() {
    }

private:
    SListItem *next() {
        return _next;
    }
    void next(SListItem *i) {
        _next = i;
    }

    SListItem *_next;
};

/**
 * Generic iterator for a singly linked list. Expects the list node class to have a next() method.
 */
template<class T>
class SListIterator {
public:
    explicit SListIterator(T *n = nullptr) : _n(n) {
    }

    T & operator*() const {
        return *_n;
    }
    T *operator->() const {
        return &operator*();
    }
    SListIterator<T>& operator++() {
        _n = static_cast<T*>(_n->next());
        return *this;
    }
    SListIterator<T> operator++(int) {
        SListIterator<T> tmp(*this);
        operator++();
        return tmp;
    }
    bool operator==(const SListIterator<T>& rhs) const {
        return _n == rhs._n;
    }
    bool operator!=(const SListIterator<T>& rhs) const {
        return _n != rhs._n;
    }

private:
    T *_n;
};

/**
 * The singly linked list. Takes an arbitrary class as list-item and expects it to have a prev(),
 * prev(T*), next() and next(*T) method. In most cases, you should inherit from SListItem and
 * specify your class for T.
 */
template<class T>
class SList {
public:
    typedef SListIterator<T> iterator;

    /**
     * Constructor. Creates an empty list
     */
    explicit SList() : _head(nullptr), _tail(nullptr), _len(0) {
    }

    /**
     * @return the number of items in the list
     */
    size_t length() const {
        return _len;
    }

    /**
     * @return beginning of list
     */
    iterator begin() const {
        return iterator(_head);
    }
    /**
     * @return end of list
     */
    iterator end() const {
        return iterator();
    }
    /**
     * @return tail of the list, i.e. the last valid item
     */
    iterator tail() const {
        return iterator(_tail);
    }

    /**
     * Appends the given item to the list. This works in constant time.
     *
     * @param e the list item
     * @return the position where it has been inserted
     */
    iterator append(T *e) {
        if(_head == nullptr)
            _head = e;
        else
            _tail->next(e);
        _tail = e;
        e->next(nullptr);
        _len++;
        return iterator(e);
    }
    /**
     * Inserts the given item into the list after <p>. This works in constant time.
     *
     * @param p the previous item (p = insert it at the beginning)
     * @param e the list item
     * @return the position where it has been inserted
     */
    iterator insert(T *p, T *e) {
        e->next(p ? p->next() : _head);
        if(p)
            p->next(e);
        else
            _head = e;
        if(!e->next())
            _tail = e;
        _len++;
        return iterator(e);
    }
    /**
     * Removes the given item from the list. This works in linear time.
     * Does NOT expect that the item is in the list!
     *
     * @param e the list item
     * @return true if the item has been found and removed
     */
    bool remove(T *e) {
        T *t = _head, *p = nullptr;
        while(t && t != e) {
            p = t;
            t = static_cast<T*>(t->next());
        }
        if(!t)
            return false;
        if(p)
            p->next(e->next());
        else
            _head = static_cast<T*>(e->next());
        if(!e->next())
            _tail = p;
        _len--;
        return true;
    }

private:
    T *_head;
    T *_tail;
    size_t _len;
};

}
