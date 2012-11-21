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

#include <util/RegionManager.h>
#include <kobj/Ports.h>

namespace nre {

class PortManager;
static OStream &operator<<(OStream &os, const PortManager &pm);

/**
 * Keeps track of I/O ports. You can either manage free ports or used ports by choosing the mode
 * of operation on construction.
 */
class PortManager {
    friend OStream &operator<<(OStream &os, const PortManager &pm);
public:
    typedef RegionManager<>::const_iterator const_iterator;

    enum Mode {
        FREE,       // manage free ports
        USED        // manage used ports
    };

    /**
     * Creates an empty port list in given mode. If mode == FREE, this object manages free ports.
     * Otherwise it manages used ports.
     *
     * @param mode the mode of operation
     */
    explicit PortManager(Mode mode) : _mode(mode), _ports() {
    }

    /**
     * @return the beginning of all port regions
     */
    const_iterator begin() const {
        return _ports.begin();
    }
    /**
     * @return the end of all port regions
     */
    const_iterator end() const {
        return _ports.end();
    }

    /**
     * Adds the ports <start> .. <count>-1 to the free/used ports.
     *
     * @param start the first port
     * @param count the number of ports
     * @throws RegionException if something failed
     */
    void add(Ports::port_t start, uint count) {
        if(_mode == FREE)
            _ports.free(start, count);
        else
            _ports.alloc_at(start, count, true);
    }
    /**
     * Removes the ports <start> .. <count>-1 from the free/used ports
     *
     * @param start the first port
     * @param count the number of ports
     * @throws RegionException if something failed
     */
    void remove(Ports::port_t start, uint count) {
        if(_mode == FREE)
            _ports.alloc_at(start, count, true);
        else
            _ports.free(start, count);
    }

private:
    Mode _mode;
    RegionManager<> _ports;
};

static inline OStream &operator<<(OStream &os, const PortManager &pm) {
    for(auto it = pm.begin(); it != pm.end(); ++it)
        os << "\t" << fmt(it->addr, "#x") << " .. " << fmt(it->addr + it->size, "#x") << "\n";
    return os;
}

}
