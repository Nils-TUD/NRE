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

#include <arch/Types.h>
#include <services/Keyboard.h>

class Keymap {
    struct KeymapEntry {
        char def;
        char shift;
        char alt;
    };

public:
    static char translate(const nre::Keyboard::Packet &in) {
        KeymapEntry *km = keymap + in.keycode;
        if(in.flags & (nre::Keyboard::LSHIFT | nre::Keyboard::RSHIFT))
            return km->shift;
        if(in.flags & (nre::Keyboard::LALT | nre::Keyboard::RALT))
            return km->alt;
        return km->def;
    }

private:
    Keymap();

    static KeymapEntry keymap[];
};
