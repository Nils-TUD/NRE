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
#include <stream/OStream.h>
#include <utcb/UtcbFrame.h>
#include <Assert.h>

namespace nre {

/**
 * Describes a DMA transfer
 */
struct DMADesc {
    /**
     * The offset in a dataspace where to transfer from/to
     */
    size_t offset;
    /**
     * The number of bytes to transfer
     */
    size_t count;

    /**
     * Creates an empty descriptor
     */
    explicit DMADesc() : offset(), count() {
    }
    /**
     * Creates a descriptor with given offset and count
     */
    explicit DMADesc(size_t offset, size_t count) : offset(offset), count(count) {
    }
};

/**
 * A list of DMA descriptors that can be easily passed around
 *
 * @param MAX the maximum number of descriptors (directly stored in the object)
 */
template<size_t MAX>
class DMADescList {
public:
    typedef const DMADesc *iterator;

    /**
     * Creates an empty list
     */
    explicit DMADescList() : _descs(), _count(0), _total(0) {
    }

    /**
     * @return the number of descriptors
     */
    size_t count() const {
        return _count;
    }
    /**
     * @return the beginning of the existing descriptors
     */
    iterator begin() const {
        return _descs;
    }
    /**
     * @return the end of the existing descriptors
     */
    iterator end() const {
        return _descs + _count;
    }

    /**
     * @return the total number of bytes in all descriptors
     */
    size_t bytecount() const {
        return _total;
    }

    /**
     * Adds the given descriptor to the list
     *
     * @param desc the descriptor
     * @return true on success
     */
    void push(const DMADesc &desc) {
        assert(_count < MAX);
        _descs[_count++] = desc;
        _total += desc.count;
    }
    /**
     * Removes the last added descriptor
     */
    void pop() {
        assert(_count > 0);
        _total -= _descs[_count - 1].count;
        _count--;
    }

    /**
     * Resets the list
     */
    void clear() {
        _count = _total = 0;
    }

    /**
     * Copies <len> bytes from <src> into <dst>. The offset specified the number of bytes you've
     * already copied (previous calls), i.e. all descriptors before that offset are skipped.
     *
     * @param dst the destination buffer
     * @param len the number of bytes to copy
     * @param offset the number of bytes already copied
     * @param src the dataspace to read from
     * @return true if successful
     */
    bool in(void *dst, size_t len, size_t offset, const DataSpace &src) const {
        return inout(dst, len, offset, src, false);
    }
    /**
     * Copies <len> bytes from <src> to <dst>. The offset specified the number of bytes you've
     * already copied (previous calls), i.e. all descriptors before that offset are skipped.
     *
     * @param src the source buffer
     * @param len the number of bytes to copy
     * @param offset the number of bytes already copied
     * @param dst the dataspace to write to
     * @return true if successful
     */
    bool out(const void *src, size_t len, size_t offset, const DataSpace &dst) const {
        return inout(const_cast<void*>(src), len, offset, dst, true);
    }

private:
    bool inout(void *buffer, size_t len, size_t offset, const DataSpace &ds, bool out) const {
        iterator it;
        char *buf = reinterpret_cast<char*>(buffer);
        for(it = begin(); it != end() && offset >= it->count; ++it)
            offset -= it->count;
        for(; len > 0 && it != end(); ++it) {
            assert(it->count >= offset);
            size_t sublen = Math::min<size_t>(it->count - offset, len);
            if((it->offset + offset) > ds.size() || (it->offset + offset + sublen) > ds.size())
                break;

            char *addr = reinterpret_cast<char*>(it->offset + ds.virt() + offset);
            if(out)
                memcpy(addr, buf, sublen);
            else
                memcpy(buf, addr, sublen);

            buf += sublen;
            len -= sublen;
            offset = 0;
        }
        return len > 0;
    }

    DMADesc _descs[MAX];
    size_t _count;
    size_t _total;
};

/**
 * Writes a string-representation into <os>
 */
template<size_t MAX>
static inline OStream &operator<<(OStream &os, const DMADescList<MAX> &l) {
    os << "DMADescList[" << l.count() << ", " << l.bytecount() << "] (";
    for(typename DMADescList<MAX>::iterator it = l.begin(); it != l.end(); ) {
        os.writef("o=%p s=%#zu", reinterpret_cast<void*>(it->offset), it->count);
        if(++it != l.end())
            os << ",";
    }
    os << ")";
    return os;
}

/**
 * Writes the list into the UTCB. Note that the UTCB size is limited!
 */
template<size_t MAX>
static inline UtcbFrameRef &operator<<(UtcbFrameRef &uf, const DMADescList<MAX> &l) {
    uf << l.count();
    for(typename DMADescList<MAX>::iterator it = l.begin(); it != l.end(); ++it)
        uf << *it;
    return uf;
}
/**
 * Reads the list back from the UTCB
 */
template<size_t MAX>
static inline UtcbFrameRef &operator>>(UtcbFrameRef &uf, DMADescList<MAX> &l) {
    size_t count;
    uf >> count;
    l.clear();
    while(count-- > 0) {
        DMADesc desc;
        uf >> desc;
        l.push(desc);
    }
    return uf;
}

}
