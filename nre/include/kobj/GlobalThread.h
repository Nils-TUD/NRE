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

#include <arch/ExecEnv.h>
#include <arch/Startup.h>
#include <kobj/Thread.h>
#include <kobj/Pd.h>
#include <Hip.h>

/**
 * The return address for GlobalThread-functions, which will terminate the Thread.
 */
EXTERN_C void ec_landing_spot(void);

namespace nre {

class Utcb;
class Sc;

/**
 * A GlobalThread is a thread that has time. That means, it is a "freely running" thread, in
 * contrast to a LocalThread which does only serve portal-calls. Note that you always have to
 * call start() to bind a Sc to it and start it.
 *
 * If you create a global thread in your Pd (the default case), the GlobalThread and Sc objects
 * are managed automatically for you. That is, you call the static create() method to create a
 * GlobalThread and call start() afterwards. When the thread is finished, i.e. the thread-function
 * returns, it will destroy itself and the Sc as well. Note that this does also mean that you
 * should be careful when accessing a GlobalThread-object of a different thread. Because it will
 * be destroyed when the thread is done. So, basically, after the start() call you should leave
 * the object alone.
 * If you create a global thread for a different Pd, you have to manage the GlobalThread object
 * yourself.
 */
class GlobalThread : public Thread {
    friend void ::_post_init();

public:
    typedef ExecEnv::startup_func startup_func;

    /**
     * Creates a new GlobalThread in the current Pd that starts at <start> on CPU <cpu>.
     *
     * @param start the entry-point of the Thread
     * @param cpu the logical CPU id to bind the Thread to
     * @param name the name of the thread
     * @param utcb the utcb-address (0 = select it automatically)
     */
    static GlobalThread *create(startup_func start, cpu_t cpu, const String &name, uintptr_t utcb = 0) {
        // note that we force a heap-allocation by this static create function, because the thread
        // will delete itself when its done.
        return new GlobalThread(start, cpu, name, Pd::current(), utcb);
    }

    /**
     * Creates a new GlobalThread that runs in a different protection domain. Thus, you have to
     * create and free this object.
     *
     * @param start the entry-point of the Thread
     * @param cpu the CPU to bind the Thread to
     * @param name the name of the thread (only used for display-purposes)
     * @param pd the protection-domain
     * @param utcb the utcb-address
     */
    explicit GlobalThread(startup_func start, cpu_t cpu, const String &name, Pd *pd, uintptr_t utcb)
        : Thread(pd, Syscalls::EC_GLOBAL, start, reinterpret_cast<uintptr_t>(ec_landing_spot), cpu,
                 Hip::get().service_caps() * cpu, 0, utcb), _sc(), _name(name) {
    }
    virtual ~GlobalThread();

    /**
     * @return the scheduling context this thread is bound to (0 if start() hasn't been called yet)
     */
    Sc *sc() const {
        return _sc;
    }
    /**
     * @return the name of this thread
     */
    const String &name() const {
        return _name;
    }

    /**
     * Starts this thread with given quantum-priority-descriptor
     *
     * @param qpd the qpd to use
     * @param pd the pd to start the thread in
     */
    void start(Qpd qpd = Qpd(), Pd *pd = Pd::current());

private:
    explicit GlobalThread(uintptr_t uaddr, capsel_t gt, capsel_t sc, cpu_t cpu, Pd *pd, uintptr_t stack);

    Sc *_sc;
    const String _name;
    static GlobalThread _cur;
};

}
