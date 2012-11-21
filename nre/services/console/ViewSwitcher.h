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

#include <ipc/Producer.h>
#include <ipc/Consumer.h>
#include <kobj/GlobalThread.h>
#include <kobj/Sc.h>
#include <mem/DataSpace.h>

#include "Screen.h"

class ConsoleService;
class ConsoleSessionData;

class ViewSwitcher {
    static const size_t DS_SIZE       = nre::ExecEnv::PAGE_SIZE * Screen::PAGES;
    static const uint COLOR           = 0x1F;
    static const uint SWITCH_TIME     = 1000; // ms
    static const uint REFRESH_DELAY   = 25;   // ms

    struct SwitchCommand {
        size_t oldsessid;
        size_t sessid;
    };

public:
    explicit ViewSwitcher(ConsoleService *srv);

    void start() {
        _ec->start();
    }

    void switch_to(ConsoleSessionData *from, ConsoleSessionData *to);

private:
    static void switch_thread(void*);

    nre::UserSm _usm;
    nre::DataSpace _ds;
    nre::Sm _sm;
    nre::Producer<SwitchCommand> _prod;
    nre::Consumer<SwitchCommand> _cons;
    nre::GlobalThread *_ec;
    ConsoleService *_srv;
    static char _backup[];
    static char _buffer[];
};
