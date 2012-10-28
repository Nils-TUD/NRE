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

namespace nre {

/**
 * RAII class for dynamic memory.
 */
template<class T>
class ScopedPtr {
public:
    /**
     * Constructor
     *
     * @param obj pointer to the object that has been allocated with new
     */
    explicit ScopedPtr(T *obj)
        : _obj(obj) {
    }
    /**
     * Destructor. Uses delete to free the object, if it hasn't been released previously.
     */
    ~ScopedPtr() {
        delete _obj;
    }

    /**
     * @return the object
     */
    T *get() {
        return _obj;
    }
    /**
     * @return the object
     */
    T *operator->() {
        return _obj;
    }

    /**
     * Specifies that the object should be kept, i.e. should not be deleted during construction
     * of this ScopedPtr object
     *
     * @return the object
     */
    T *release() {
        T *res = _obj;
        _obj = 0;
        return res;
    }

private:
    ScopedPtr(const ScopedPtr&);
    ScopedPtr& operator=(const ScopedPtr&);

    T *_obj;
};

}
