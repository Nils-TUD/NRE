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

#include <ipc/Service.h>
#include <stream/Serial.h>
#include <kobj/Ports.h>
#include <kobj/Sm.h>

class BufferedLog;

/**
 * The log implementation that provides a service for child tasks that allows them to print lines
 * to the serial line.
 */
class Log : public nre::BaseSerial {
    friend class BufferedLog;

    enum {
        COM1    = 0x3F8,
        COM2    = 0x2E8,
        COM3    = 0x2F8,
        COM4    = 0x3E8
    };
    enum {
        DLR_LO  = 0,
        DLR_HI  = 1,
        IER     = 1,    // interrupt enable register
        FCR     = 2,    // FIFO control register
        LCR     = 3,    // line control register
        MCR     = 4,    // modem control register
    };

    static const uint PORT_BASE     = COM1;
    static const uint ROOT_SESS     = 0;

public:
    /**
     * @return the instance
     */
    static Log &get() {
        return _inst;
    }
    /**
     * Starts the log service
     */
    void start();

private:
    explicit Log();

    void write(uint sessid, const char *line, size_t len);

    virtual void write(char c) {
        if(c == '\0')
            return;

        if(c == '\n')
            write('\r');
        while((_ports.in<uint8_t>(5) & 0x20) == 0)
            ;
        _ports.out<uint8_t>(c, 0);
    }

    // note that we use a portal here instead of shared-memory because dataspace sharing doesn't
    // work with services living in root. the problem is the translation of caps. the translation
    // stops as soon as the destination Pd is reached. since stuff in root walks to the
    // directly to the root-ds-manager and bypasses the childmanager, we receive the cap that is
    // actually meant for the childmanager in the root-ds-manager. thus, we don't find the dataspace.
    // to prevent the whole problem, we use a portal which works fine in this case, since its
    // not performance critical anyway. so, sending a long string doesn't really hurt.
    PORTAL static void portal(capsel_t pid);

    nre::Ports _ports;
    nre::UserSm _sm;
    bool _ready;
    static Log _inst;
    static nre::Service *_srv;
    static const char *_colors[];
};

/**
 * This class is used by root itself which buffers the serial output line-wise and uses the same
 * backend as the log-service to finally write lines to the serial line.
 * Note that this class is not intended to be used directly. Please use Serial::get() in root, like
 * you do in all other child tasks as well.
 */
class BufferedLog : public nre::BaseSerial {
    explicit BufferedLog();

    virtual void write(char c) {
        if(c == '\0')
            return;

        if(_bufpos == sizeof(_buf) || c == '\n') {
            // take care that the log is already initialized
            if(Log::get()._ready)
                Log::get().write(Log::ROOT_SESS, _buf, _bufpos);
            _bufpos = 0;
        }
        if(c != '\n')
            _buf[_bufpos++] = c;
    }

    size_t _bufpos;
    char _buf[MAX_LINE_LEN + 1];
    static BufferedLog _inst;
};
