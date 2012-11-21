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

#include <RCU.h>
#include <cstring>

#include "ConsoleSessionData.h"

using namespace nre;

void ConsoleSessionData::create(DataSpace *in_ds, DataSpace *out_ds, Sm *sm, size_t con,
                                const String &title) {
    ScopedLock<UserSm> guard(&_sm);
    if(_in_ds != nullptr)
        throw Exception(E_EXISTS, "Console session already initialized");
    if(con >= Console::SUBCONS)
        VTHROW(Exception, E_ARGS_INVALID, "Subconsole " << con << " does not exist");
    _in_ds = in_ds;
    _out_ds = out_ds;
    _in_sm = sm;
    if(_in_ds)
        _prod = new Producer<Console::ReceivePacket>(*in_ds, *sm, false);
    _console = con;
    _title = title;
    _srv->session_ready(this);
}

void ConsoleSessionData::portal(capsel_t pid) {
    UtcbFrameRef uf;
    try {
        ScopedLock<RCULock> guard(&RCU::lock());
        ConsoleService *srv = Thread::current()->get_tls<ConsoleService*>(Thread::TLS_PARAM);
        ConsoleSessionData *sess = srv->get_session<ConsoleSessionData>(pid);
        Console::Command cmd;
        uf >> cmd;
        switch(cmd) {
            case Console::CREATE: {
                size_t con;
                String title;
                capsel_t insel = uf.get_delegated(0).offset();
                capsel_t outsel = uf.get_delegated(0).offset();
                capsel_t smsel = uf.get_delegated(0).offset();
                uf >> con >> title;
                uf.finish_input();

                sess->create(new DataSpace(insel), new DataSpace(outsel), new Sm(smsel, false),
                             con, title);
                uf.accept_delegates();
                uf << E_SUCCESS;
            }
            break;

            case Console::GET_REGS: {
                uf.finish_input();

                uf << E_SUCCESS << sess->regs();
            }
            break;

            case Console::SET_REGS: {
                Console::Register regs;
                uf >> regs;
                uf.finish_input();

                sess->set_regs(regs);
                uf << E_SUCCESS;
            }
            break;
        }
    }
    catch(const Exception &e) {
        Syscalls::revoke(uf.delegation_window(), true);
        uf.clear();
        uf << e;
    }
}
