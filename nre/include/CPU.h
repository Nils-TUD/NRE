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

#include <kobj/Thread.h>
#include <util/SList.h>
#include <Assert.h>
#include <Hip.h>

namespace nre {

class Pt;
class CPUInit;

/**
 * The CPU class is used to get the current CPU, iterate through all online CPUs and it holds
 * the parent portals, that are used to allocate or release certain resources.
 * You can't create instances of it, because that is done automatically during startup.
 * Every CPU has a logical and physical id. The physical id is the index in the Hip, where it
 * appears. The logical id is assigned from 0..(n-1).
 */
class CPU {
    friend class CPUInit;
    friend class SListIterator<CPU>;

public:
    typedef SListIterator<CPU> iterator;

    /**
     * @return the current CPU, i.e. the CPU you're running on
     */
    static CPU &current() {
        return get(Thread::current()->cpu());
    }
    /**
     * @param log_id the logical CPU id
     * @return the CPU with given id
     */
    static CPU &get(cpu_t log_id) {
        assert(log_id < Hip::MAX_CPUS);
        return _cpus[log_id];
    }

    /**
     * @return the number of online CPUs
     */
    static size_t count() {
        return _count;
    }
    /**
     * @return the next order of online CPUs, i.e. Math::next_pow2_shift(count())
     */
    static uint order() {
        return _order;
    }

    /**
     * @return iterator beginning of the list of online CPUs. They are always sorted by logical id.
     */
    static iterator begin() {
        return SListIterator<CPU>(_online);
    }
    /**
     * @return iterator end of the list of online CPUs
     */
    static iterator end() {
        return SListIterator<CPU>();
    }

    /**
     * @return flags of this CPU
     */
    uint8_t flags() const {
        return _flags;
    }
    /**
     * @return the thread of this CPU (hyperthread)
     */
    uint8_t thread() const {
        return _thread;
    }
    /**
     * @return the core of this CPU
     */
    uint8_t core() const {
        return _core;
    }
    /**
     * @return the package of this CPU
     */
    uint8_t package() const {
        return _package;
    }

    /**
     * @return the physical CPU id
     */
    cpu_t phys_id() const {
        return _logtophys[_id];
    }
    /**
     * @return the logical CPU id
     */
    cpu_t log_id() const {
        return _id;
    }

    /**
     * The dataspace portal
     */
    Pt &ds_pt() {
        return *_ds_pt;
    }
    void ds_pt(Pt *pt) {
        _ds_pt = pt;
    }
    /**
     * The I/O portal
     */
    Pt &io_pt() {
        return *_io_pt;
    }
    void io_pt(Pt *pt) {
        _io_pt = pt;
    }
    /**
     * The GSI portal
     */
    Pt &gsi_pt() {
        return *_gsi_pt;
    }
    void gsi_pt(Pt *pt) {
        _gsi_pt = pt;
    }
    /**
     * The service portal
     */
    Pt &srv_pt() {
        return *_srv_pt;
    }
    void srv_pt(Pt *pt) {
        _srv_pt = pt;
    }
    /**
     * The Sc portal
     */
    Pt &sc_pt() {
        return *_sc_pt;
    }
    void sc_pt(Pt *pt) {
        _sc_pt = pt;
    }

private:
    CPU()
        : _id(), _next(), _flags(), _thread(), _core(), _package(), _ds_pt(), _io_pt(),
          _gsi_pt(), _srv_pt(), _sc_pt() {
    }
    CPU(const CPU&);
    CPU& operator=(const CPU&);

    CPU *next() {
        return _next;
    }

    cpu_t _id;
    CPU *_next;
    uint8_t _flags;
    uint8_t _thread;
    uint8_t _core;
    uint8_t _package;
    Pt *_ds_pt;
    Pt *_io_pt;
    Pt *_gsi_pt;
    Pt *_srv_pt;
    Pt *_sc_pt;
    static size_t _count;
    static uint _order;
    static CPU *_online;
    static CPU _cpus[Hip::MAX_CPUS];
    static cpu_t _logtophys[Hip::MAX_CPUS];
};

}
