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

#include <kobj/Sm.h>
#include <mem/DataSpace.h>
#include <util/Sync.h>
#include <util/Math.h>

namespace nre {

template<typename T>
class Producer;

/**
 * Consumer-part for the producer-consumer-communication over a dataspace.
 *
 * Usage-example:
 * Consumer<char> cons(&ds);
 * for(char *c; (c = cons->get()) != nullptr; cons.next()) {
 *   // do something with *c
 * }
 */
template<typename T>
class Consumer {
    friend class Producer<T>;

    struct Interface {
        volatile size_t rpos;
        volatile size_t wpos;
        T buffer[];
    };

public:
    /**
     * Creates a consumer that uses the given dataspace for communication
     *
     * @param ds the dataspace
     * @param init whether the consumer should init the state. this should only be done by one
     *  party and preferably by the first one. That is, if the client is the consumer it should
     *  init it (because it will create the dataspace and share it to the service).
     */
    explicit Consumer(DataSpace *ds, bool init = false)
        : _ds(ds), _if(reinterpret_cast<Interface*>(ds->virt())),
          _max(Math::prev_pow2((ds->size() - sizeof(Interface)) / sizeof(T))),
          _sm(_ds->sel(), true), _stop(false) {
        if(init) {
            _if->rpos = 0;
            _if->wpos = 0;
        }
    }

    /**
     * @return the length of the ring-buffer
     */
    size_t rblength() const {
        return _max;
    }

    /**
     * Stops waiting for the producer. This way, if get() is blocked on the semaphore, it will
     * be unblocked.
     */
    void stop() {
        _stop = true;
        Sync::memory_barrier();
        try {
            _sm.up();
        }
        catch(...) {
            // ignore it
        }
    }

    /**
     * @return whether there is more data to read
     */
    bool has_data() const {
        return _if->rpos != _if->wpos;
    }

    /**
     * Retrieves the item at current position. If there is no item anymore, it blocks until the
     * producer notifies it, that there is data available. You might interrupt that by using stop().
     * Note that the method will only return 0 if it has been stopped *and* there is no data anymore.
     *
     * Important: You have to call next() to move to the next item.
     *
     * @return pointer to the data
     */
    T *get() {
        while(EXPECT_FALSE(_if->rpos == _if->wpos)) {
            if(EXPECT_FALSE(_stop))
                return nullptr;
            // they might fail if someone revokes the Sm-caps
            try {
                _sm.zero();
            }
            catch(...) {
                return nullptr;
            }
        }
        return _if->buffer + _if->rpos;
    }

    /**
     * Tells the producer that you're done working with the current item (i.e. the producer will
     * never touch the item while you're working with it)
     */
    void next() {
        _if->rpos = (_if->rpos + 1) & (_max - 1);
    }

private:
    DataSpace *_ds;
    Interface *_if;
    size_t _max;
    Sm _sm;
    bool _stop;
};

}
