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

#include "VMConfig.h"

using namespace nre;

void VMConfig::find_mods(size_t len) {
    const char *lines = reinterpret_cast<const char*>(_ds.virt());
    const char *end = lines + len;
    while(lines < end) {
        size_t space = 0;
        ssize_t pos;
        for(pos = 0; pos < end - lines; ++pos) {
            if(!space && lines[pos] == ' ')
                space = pos;
            else if(lines[pos] == '\n')
                break;
        }
        if(space == 0)
            space = pos;
        _mods.append(new Module(lines, space, pos));
        lines += pos + 1;
    }
}

Child::id_type VMConfig::start(ChildManager &cm, size_t console, cpu_t cpu) {
    static char args[MAX_ARGS_LEN];
    SList<Module>::iterator first = _mods.begin();
    OStringStream os(args, sizeof(args));
    os << first->args() << " console:" << console << " constitle:" << _name;

    VMChildConfig cfg(_mods, args, cpu);
    Hip::mem_iterator mod = get_module(first->name());
    DataSpace ds(mod->size, DataSpaceDesc::ANONYMOUS, DataSpaceDesc::R, mod->addr);
    return cm.load(ds.virt(), mod->size, cfg);
}

Hip::mem_iterator VMConfig::get_module(const String &name) {
    const Hip &hip = Hip::get();
    for(Hip::mem_iterator mem = hip.mem_begin(); mem != hip.mem_end(); ++mem) {
        if(strcmp(mem->cmdline(), name.str()) == 0)
            return mem;
    }
    VTHROW(Exception, E_NOT_FOUND, "Unable to find module '" << name << "'");
}

OStream & operator<<(OStream &os, const VMConfig &cfg) {
    os << "VMConfig[" << cfg.name() << "]:\n";
    for(SList<VMConfig::Module>::iterator it = cfg._mods.begin(); it != cfg._mods.end(); ++it)
        os << "  name='" << it->name() << "' args='" << it->args() << "'\n";
    return os;
}
