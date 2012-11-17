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

#include <stream/OStream.h>
#include <cstdlib>
#include <cstring>

namespace nre {

/**
 * Outputstream that writes to a string
 */
class OStringStream : public OStream {
    static const size_t DEFAULT_SIZE    = 64;

public:
    /**
     * Constructor that allocates and automatically increases the string while
     * writing to the stream
     */
    explicit OStringStream()
        : OStream(), _dynamic(true), _dst(static_cast<char*>(malloc(DEFAULT_SIZE))),
          _max(_dst ? DEFAULT_SIZE : 0), _pos() {
    }

    /**
     * Constructor that writes into the given string
     *
     * @param dst the string
     * @param max the size of <dst>
     */
    explicit OStringStream(char *dst, size_t max)
        : OStream(), _dynamic(false), _dst(dst), _max(max), _pos() {
    }

    /**
     * Destroys the string, if it has been allocated here
     */
    virtual ~OStringStream() {
        if(_dynamic)
            free(_dst);
    }

    /**
     * @return the length of the string
     */
    size_t length() const {
        return _pos;
    }
    /**
     * @return the string
     */
    const char *str() const {
        return _dst ? _dst : "";
    }

private:
    virtual void write(char c) {
        // increase the buffer, if necessary
        if(_pos + 1 >= _max && _dynamic) {
            char *ndst = static_cast<char*>(realloc(_dst, _max * 2));
            if(ndst) {
                _max *= 2;
                _dst = ndst;
            }
        }
        // write into the buffer if there is still enough room
        if(_pos + 1 < _max) {
            _dst[_pos++] = c;
            _dst[_pos] = '\0';
        }
    }

    bool _dynamic;
    char *_dst;
    size_t _max;
    size_t _pos;
};

}
