/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2007-2010, Bernhard Kauer <bk@vmmon.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NUL.
 *
 * NUL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NUL is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#include <Logging.h>

#include "HostKeyboard.h"

using namespace nre;

HostKeyboard::ScanCodeEntry HostKeyboard::scset2[] = {
    /* 00 */ {0,                         0,                          0},
    /* 01 */ {Keyboard::VK_F9,           0,                          0},
    /* 02 */ {0,                         0,                          0},
    /* 03 */ {Keyboard::VK_F5,           0,                          0},
    /* 04 */ {Keyboard::VK_F3,           0,                          0},
    /* 05 */ {Keyboard::VK_F1,           0,                          0},
    /* 06 */ {Keyboard::VK_F2,           0,                          0},
    /* 07 */ {Keyboard::VK_F12,          0,                          0},
    /* 08 */ {0,                         0,                          0},
    /* 09 */ {Keyboard::VK_F10,          0,                          0},
    /* 0A */ {Keyboard::VK_F8,           0,                          0},
    /* 0B */ {Keyboard::VK_F6,           0,                          0},
    /* 0C */ {Keyboard::VK_F4,           0,                          0},
    /* 0D */ {Keyboard::VK_TAB,          0,                          0},
    /* 0E */ {Keyboard::VK_ACCENT,       0,                          0},
    /* 0F */ {0,                         0,                          0},
    /* 10 */ {0,                         0,                          0},
    /* 11 */ {0,                         Keyboard::VK_RALT,          0},
    /* 12 */ {Keyboard::VK_LSHIFT,       0,                          0},
    /* 13 */ {0,                         0,                          0},
    /* 14 */ {Keyboard::VK_LCTRL,        Keyboard::VK_RCTRL,         Keyboard::VK_PAUSE},
    /* 15 */ {Keyboard::VK_Q,            0,                          0},
    /* 16 */ {Keyboard::VK_1,            0,                          0},
    /* 17 */ {0,                         0,                          0},
    /* 18 */ {0,                         0,                          0},
    /* 19 */ {0,                         0,                          0},
    /* 1A */ {Keyboard::VK_Z,            0,                          0},
    /* 1B */ {Keyboard::VK_S,            0,                          0},
    /* 1C */ {Keyboard::VK_A,            0,                          0},
    /* 1D */ {Keyboard::VK_W,            0,                          0},
    /* 1E */ {Keyboard::VK_2,            0,                          0},
    /* 1F */ {0,                         Keyboard::VK_LWIN,          0},
    /* 20 */ {0,                         0,                          0},
    /* 21 */ {Keyboard::VK_C,            0,                          0},
    /* 22 */ {Keyboard::VK_X,            0,                          0},
    /* 23 */ {Keyboard::VK_D,            0,                          0},
    /* 24 */ {Keyboard::VK_E,            0,                          0},
    /* 25 */ {Keyboard::VK_4,            0,                          0},
    /* 26 */ {Keyboard::VK_3,            0,                          0},
    /* 27 */ {0,                         Keyboard::VK_RWIN,          0},
    /* 28 */ {0,                         0,                          0},
    /* 29 */ {Keyboard::VK_SPACE,        0,                          0},
    /* 2A */ {Keyboard::VK_V,            0,                          0},
    /* 2B */ {Keyboard::VK_F,            0,                          0},
    /* 2C */ {Keyboard::VK_T,            0,                          0},
    /* 2D */ {Keyboard::VK_R,            0,                          0},
    /* 2E */ {Keyboard::VK_5,            0,                          0},
    /* 2F */ {0,                         0,                          0},
    /* 30 */ {0,                         0,                          0},
    /* 31 */ {Keyboard::VK_N,            0,                          0},
    /* 32 */ {Keyboard::VK_B,            0,                          0},
    /* 33 */ {Keyboard::VK_H,            0,                          0},
    /* 34 */ {Keyboard::VK_G,            0,                          0},
    /* 35 */ {Keyboard::VK_Y,            0,                          0},
    /* 36 */ {Keyboard::VK_6,            0,                          0},
    /* 37 */ {0,                         0,                          0},
    /* 38 */ {0,                         0,                          0},
    /* 39 */ {0,                         0,                          0},
    /* 3A */ {Keyboard::VK_M,            0,                          0},
    /* 3B */ {Keyboard::VK_J,            0,                          0},
    /* 3C */ {Keyboard::VK_U,            0,                          0},
    /* 3D */ {Keyboard::VK_7,            0,                          0},
    /* 3E */ {Keyboard::VK_8,            0,                          0},
    /* 3F */ {0,                         0,                          0},
    /* 40 */ {0,                         0,                          0},
    /* 41 */ {Keyboard::VK_COMMA,        0,                          0},
    /* 42 */ {Keyboard::VK_K,            0,                          0},
    /* 43 */ {Keyboard::VK_I,            0,                          0},
    /* 44 */ {Keyboard::VK_O,            0,                          0},
    /* 45 */ {Keyboard::VK_0,            0,                          0},
    /* 46 */ {Keyboard::VK_9,            0,                          0},
    /* 47 */ {0,                         0,                          0},
    /* 48 */ {0,                         0,                          0},
    /* 49 */ {Keyboard::VK_DOT,          0,                          0},
    /* 4A */ {Keyboard::VK_SLASH,        Keyboard::VK_KPDIV,         0},
    /* 4B */ {Keyboard::VK_L,            0,                          0},
    /* 4C */ {Keyboard::VK_SEM,          0,                          0},
    /* 4D */ {Keyboard::VK_P,            0,                          0},
    /* 4E */ {Keyboard::VK_MINUS,        0,                          0},
    /* 4F */ {0,                         0,                          0},
    /* 50 */ {0,                         0,                          0},
    /* 51 */ {0,                         0,                          0},
    /* 52 */ {Keyboard::VK_APOS,         0,                          0},
    /* 53 */ {0,                         0,                          0},
    /* 54 */ {Keyboard::VK_LBRACKET,     0,                          0},
    /* 55 */ {Keyboard::VK_EQ,           0,                          0},
    /* 56 */ {0,                         0,                          0},
    /* 57 */ {0,                         0,                          0},
    /* 58 */ {Keyboard::VK_CAPS,         0,                          0},
    /* 59 */ {Keyboard::VK_RSHIFT,       0,                          0},
    /* 5A */ {Keyboard::VK_ENTER,        Keyboard::VK_KPENTER,       0},
    /* 5B */ {Keyboard::VK_RBRACKET,     0,                          0},
    /* 5C */ {0,                         0,                          0},
    /* 5D */ {Keyboard::VK_BACKSLASH,    0,                          0},
    /* 5E */ {0,                         0,                          0},
    /* 5F */ {0,                         0,                          0},
    /* 60 */ {0,                         0,                          0},
    /* 61 */ {0,                         0,                          0},
    /* 62 */ {0,                         0,                          0},
    /* 63 */ {0,                         0,                          0},
    /* 64 */ {0,                         0,                          0},
    /* 65 */ {0,                         0,                          0},
    /* 66 */ {Keyboard::VK_BACKSP,       0,                          0},
    /* 67 */ {0,                         0,                          0},
    /* 68 */ {0,                         0,                          0},
    /* 69 */ {Keyboard::VK_KP1,          Keyboard::VK_END,           0},
    /* 6A */ {0,                         0,                          0},
    /* 6B */ {Keyboard::VK_KP4,          Keyboard::VK_LEFT,          0},
    /* 6C */ {Keyboard::VK_KP7,          Keyboard::VK_HOME,          0},
    /* 6D */ {0,                         0,                          0},
    /* 6E */ {0,                         0,                          0},
    /* 6F */ {0,                         0,                          0},
    /* 70 */ {Keyboard::VK_KP0,          Keyboard::VK_INSERT,        0},
    /* 71 */ {Keyboard::VK_KPDOT,        Keyboard::VK_DELETE,        0},
    /* 72 */ {Keyboard::VK_KP2,          Keyboard::VK_DOWN,          0},
    /* 73 */ {Keyboard::VK_KP5,          0,                          0},
    /* 74 */ {Keyboard::VK_KP6,          Keyboard::VK_RIGHT,         0},
    /* 75 */ {Keyboard::VK_KP8,          Keyboard::VK_UP,            0},
    /* 76 */ {Keyboard::VK_ESC,          0,                          0},
    /* 77 */ {Keyboard::VK_NUM,          0,                          Keyboard::VK_PAUSE},
    /* 78 */ {Keyboard::VK_F11,          0,                          0},
    /* 79 */ {Keyboard::VK_KPADD,        0,                          0},
    /* 7A */ {Keyboard::VK_KP3,          Keyboard::VK_PGDOWN,        0},
    /* 7B */ {0,                         0,                          0},
    /* 7C */ {Keyboard::VK_KPMUL,        Keyboard::VK_PRINT,         0},
    /* 7D */ {Keyboard::VK_KP9,          Keyboard::VK_PGUP,          0},
    /* 7E */ {Keyboard::VK_SCROLL,       0,                          0},
    /* 7F */ {0,                         0,                          0},
    /* 80 */ {0,                         0,                          0},
    /* 81 */ {0,                         0,                          0},
    /* 82 */ {0,                         0,                          0},
    /* 83 */ {Keyboard::VK_F7,           0,                          0},
    /* 84 */ {0,           0,                          0},
    /* 85 */ {0,                         0,                          0},
    /* 86 */ {0,                         0,                          0},
    /* 87 */ {0,                         0,                          0},
    /* 88 */ {0,                         0,                          0},
    /* 89 */ {0,                         0,                          0},
    /* 8A */ {0,                         0,                          0},
    /* 8B */ {0,                         0,                          0},
    /* 8C */ {0,                         0,                          0},
    /* 8D */ {0,                         0,                          0},
    /* 8E */ {0,                         0,                          0},
    /* 8F */ {0,                         0,                          0},
};

uint8_t HostKeyboard::sc1_to_sc2(uint8_t scancode) {
    static const uint8_t map[128] = {
        0x00, 0x76, 0x16, 0x1e, 0x26, 0x25, 0x2e, 0x36, 0x3d, 0x3e, 0x46, 0x45, 0x4e, 0x55, 0x66, 0x0d,
        0x15, 0x1d, 0x24, 0x2d, 0x2c, 0x35, 0x3c, 0x43, 0x44, 0x4d, 0x54, 0x5b, 0x5a, 0x14, 0x1c, 0x1b,
        0x23, 0x2b, 0x34, 0x33, 0x3b, 0x42, 0x4b, 0x4c, 0x52, 0x0e, 0x12, 0x5d, 0x1a, 0x22, 0x21, 0x2a,
        0x32, 0x31, 0x3a, 0x41, 0x49, 0x4a, 0x59, 0x7c, 0x11, 0x29, 0x58, 0x05, 0x06, 0x04, 0x0c, 0x03,
        0x0b, 0x83, 0x0a, 0x01, 0x09, 0x77, 0x7e, 0x6c, 0x75, 0x7d, 0x7b, 0x6b, 0x73, 0x74, 0x79, 0x69,
        0x72, 0x7a, 0x70, 0x71, 0x84, 0x60, 0x61, 0x78, 0x07, 0x0f, 0x17, 0x1f, 0x27, 0x2f, 0x37, 0x3f,
        0x47, 0x4f, 0x56, 0x5e, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0x40, 0x48, 0x50, 0x57, 0x6f,
        0x13, 0x19, 0x39, 0x51, 0x53, 0x5c, 0x5f, 0x62, 0x63, 0x64, 0x65, 0x67, 0x68, 0x6a, 0x6d, 0x6e
    };
    switch(scancode) {
        case 0xff:
            return 0x00;
        case 0xe1:
        case 0xe0:
            return scancode;
        default:
            return map[scancode & 0x7f];
    }
}

bool HostKeyboard::wait_status(uint8_t mask, uint8_t value) {
    timevalue_t timeout = _clock.source_time(TIMEOUT);
    uint8_t status;
    do
        status = _port_ctrl.in<uint8_t>();
    while((status & mask) != value && _clock.source_time() < timeout);
    return (status & mask) == value;
}

bool HostKeyboard::wait_ack() {
    uint8_t status;
    timevalue_t timeout = _clock.source_time(TIMEOUT);
    do {
        status = _port_ctrl.in<uint8_t>();
        if((status & STATUS_DATA_AVAIL) && _port_data.in<uint8_t>() == ACK
           /* && (~status & STATUS_MOUSE_DATA_AVAIL)*/)
            return true;
    }
    while(_clock.source_time() < timeout);
    return false;
}

bool HostKeyboard::wait_output_full() {
    return wait_status(STATUS_DATA_AVAIL, STATUS_DATA_AVAIL);
}

bool HostKeyboard::wait_input_empty() {
    return wait_status(STATUS_BUSY, 0);
}

bool HostKeyboard::disable_devices() {
    if(!wait_input_empty())
        return false;
    _port_ctrl.out<uint8_t>(KBC_CMD_DISABLE_KEYBOARD);
    if(!wait_input_empty())
        return false;
    _port_ctrl.out<uint8_t>(KBC_CMD_DISABLE_MOUSE);
    // drop output buffer?
    while(_port_ctrl.in<uint8_t>() & STATUS_DATA_AVAIL)
        _port_data.in<uint8_t>();
    if(!wait_input_empty())
        return false;
    assert(!(_port_ctrl.in<uint8_t>() & STATUS_DATA_AVAIL));
    return true;
}

bool HostKeyboard::enable_devices() {
    if(!wait_input_empty())
        return false;
    _port_ctrl.out<uint8_t>(KBC_CMD_ENABLE_KEYBOARD);
    if(!wait_input_empty())
        return false;
    _port_ctrl.out<uint8_t>(KBC_CMD_ENABLE_MOUSE);
    if(!wait_input_empty())
        return false;
    return true;
}

void HostKeyboard::enable_mouse() {
    // put mouse in streaming mode
    if(!write_mouse_ack(MOUSE_CMD_STREAMING))
        LOG(Logging::KEYBOARD, Serial::get().writef("kb: %s() failed at %d\n", __func__, __LINE__));

    // enable mouse-wheel by setting sample-rate to 200, 100 and 80 and reading the device-id
    write_mouse_ack(MOUSE_CMD_SETSAMPLE);
    write_mouse_ack(200);
    write_mouse_ack(MOUSE_CMD_SETSAMPLE);
    write_mouse_ack(100);
    write_mouse_ack(MOUSE_CMD_SETSAMPLE);
    write_mouse_ack(80);
    write_mouse_ack(MOUSE_CMD_GETDEVID);
    wait_output_full();
    uint8_t id = _port_data.in<uint8_t>();
    if(id == 3 || id == 4)
        _wheel = true;
}

bool HostKeyboard::read_cmd(uint8_t cmd, uint8_t &value) {
    if(!wait_input_empty())
        return false;
    _port_ctrl.out<uint8_t>(cmd);
    if(!wait_output_full())
        return false;
    value = _port_data.in<uint8_t>();
    return true;
}

bool HostKeyboard::write_cmd(uint8_t cmd, uint8_t value) {
    if(!wait_input_empty())
        return false;
    _port_ctrl.out<uint8_t>(cmd);
    if(!wait_input_empty())
        return false;
    _port_data.out<uint8_t>(value);
    return true;
}

bool HostKeyboard::write_keyboard_ack(uint8_t value) {
    if(!wait_input_empty())
        return false;
    _port_data.out<uint8_t>(value);
    return wait_ack();
}

bool HostKeyboard::write_mouse_ack(uint8_t value) {
    if(!wait_input_empty())
        return false;
    write_cmd(KBC_CMD_NEXT2MOUSE, value);
    return wait_ack();
}

bool HostKeyboard::read(Keyboard::Packet &data) {
    uint8_t status;
    while((status = _port_ctrl.in<uint8_t>()) & STATUS_DATA_AVAIL) {
        if(status & STATUS_MOUSE_DATA_AVAIL)
            return false;
        uint8_t sc = _port_data.in<uint8_t>();
        if(handle_scancode(data, sc))
            return true;
    }
    return false;
}

bool HostKeyboard::read(Mouse::Packet &data) {
    uint8_t status;
    while((status = _port_ctrl.in<uint8_t>()) & STATUS_DATA_AVAIL) {
        if(~status & STATUS_MOUSE_DATA_AVAIL)
            return false;
        uint8_t sc = _port_data.in<uint8_t>();
        if(handle_aux(data, sc))
            return true;
    }
    return false;
}

bool HostKeyboard::handle_aux(Mouse::Packet &data, uint8_t byte) {
    switch(_mousestate) {
        // wait for reset ack
        case 0xfe:
            if(byte == 0xaa)
                _mousestate++;
            else {
                LOG(Logging::KEYBOARD, Serial::get().writef(
                        "kb: %s no reset ack %x\n", __func__, byte));
            }
            return false;

        // wait for reset id
        case 0xff:
            if(byte == 0) {
                _mousestate = 0;
                enable_mouse();
            }
            else {
                LOG(Logging::KEYBOARD, Serial::get().writef(
                        "kb: %s unknown mouse id %x\n", __func__, byte));
            }
            return false;

        // first byte of packet
        case 0:
            // not in sync?
            if(~byte & 0x8) {
                LOG(Logging::KEYBOARD, Serial::get().writef(
                        "kb: %s mouse not in sync - drop %x\n", __func__, byte));
                return false;
            }
            data.status = byte;
            _mousestate++;
            return false;
        case 1:
            data.x = byte;
            _mousestate++;
            return false;
        case 2:
            data.y = byte;
            if(_wheel) {
                _mousestate++;
                return false;
            }
            else
                _mousestate = 0;
            break;
        case 3:
            data.z = byte;
            _mousestate = 0;
            break;
    }
    return true;
}

bool HostKeyboard::handle_scancode(Keyboard::Packet &data, uint8_t key) {
    /**
     * There are some bad BIOSes around which do not emulate SC2.
     * We have to convert from SC1.
     */
    if(_scset1) {
        if((key & 0x80) && key != 0xe0 && key != 0xe1)
            _flags |= Keyboard::RELEASE;
        key = sc1_to_sc2(key);
    }

    /**
     * We have a small state machine here, as the keyboard runs with
     * scancode set 2. SCS3 would be much nicer but is not available
     * everywhere.
     */
    switch(key) {
        case 0x00: // overrun
            _flags &= Keyboard::NUM;
            return false;
        case 0xE0:
            _flags |= Keyboard::EXTEND0;
            return false;
        case 0xE1:
            _flags |= Keyboard::EXTEND1;
            return false;
        case 0xF0:
            _flags |= Keyboard::RELEASE;
            return false;
    }

    uint nflag = 0;
    ScanCodeEntry *e = scset2 + (key % 0x90);
    data.keycode = (_flags & Keyboard::EXTEND1) ? e->ext1 :
                   (_flags & Keyboard::EXTEND0) ? e->ext0 : e->def;
    data.scancode = key;
    switch(data.keycode) {
        case Keyboard::VK_LALT:
            nflag = Keyboard::LALT;
            break;
        case Keyboard::VK_RALT:
            nflag = Keyboard::RALT;
            break;
        case Keyboard::VK_LSHIFT:
        case Keyboard::VK_RSHIFT:
            if(_flags & Keyboard::EXTEND0) {
                // ignore extended modifiers here
                _flags &= ~(Keyboard::EXTEND0 | Keyboard::RELEASE);
                return false;
            }
            nflag = data.keycode == Keyboard::VK_LSHIFT ? Keyboard::LSHIFT : Keyboard::RSHIFT;
            break;
        case Keyboard::VK_LCTRL:
            nflag = Keyboard::LCTRL;
            break;
        case Keyboard::VK_RCTRL:
            nflag = Keyboard::RCTRL;
            break;
        case Keyboard::VK_LWIN:
            nflag = Keyboard::LWIN;
            break;
        case Keyboard::VK_RWIN:
            nflag = Keyboard::RWIN;
            break;
        case Keyboard::VK_NUM:
            if(!(_flags & (Keyboard::EXTEND1 | Keyboard::RELEASE))) {
                _flags ^= Keyboard::NUM;
                write_keyboard_ack(0xed) && write_keyboard_ack((_flags & Keyboard::NUM) ? 0x2 : 0);
            }
            break;
    }

    // the-break-key - drop the break keycodes
    if((_flags & Keyboard::EXTEND0) && (~_flags & Keyboard::RELEASE) && key == 0x7e)
        return false;

    // the normal pause key - drop the break keycodes
    if((_flags & Keyboard::EXTEND1) && ((key == 0x14) || (key == 0x77 && (~_flags & Keyboard::RELEASE))))
        return false;

    // toggle modifier
    if(_flags & Keyboard::RELEASE)
        _flags &= ~nflag;
    else
        _flags |= nflag;

    data.flags = _flags;
    _flags &= ~(Keyboard::RELEASE | Keyboard::EXTEND0 | Keyboard::EXTEND1);
    return true;
}

void HostKeyboard::reboot() {
    _port_ctrl.out<uint8_t>(0xfe);
}

void HostKeyboard::reset() {
    uint8_t cmdbyte = 0;

#if 0
    if(!disable_devices())
        LOG("kb: %s() failed at %d with %x\n", __func__, __LINE__, inb(_base + 4));
#endif

    // wait for input buffer empty
    wait_input_empty();

    // clear keyboard buffer
    while(_port_ctrl.in<uint8_t>() & STATUS_DATA_AVAIL)
        _port_data.in<uint8_t>();

    if(!read_cmd(KBC_CMD_READ_STATUS, cmdbyte))
        LOG(Logging::KEYBOARD, Serial::get().writef("kb: %s() failed at %d\n", __func__, __LINE__));

    cmdbyte &= ~KBC_CMDBYTE_TRANSPSAUX;
    // we enable translation if scset == 1
    if(_scset1)
        cmdbyte |= KBC_CMDBYTE_TRANSPSAUX;

    // set translation and enable irqs
    if(!write_cmd(KBC_CMD_SET_STATUS, cmdbyte | KBC_CMDBYTE_IRQ1 | KBC_CMDBYTE_IRQ2) ||
       !read_cmd(KBC_CMD_READ_STATUS, cmdbyte)) {
        LOG(Logging::KEYBOARD, Serial::get().writef("kb: %s() failed at %d\n", __func__, __LINE__));
    }
    _scset1 |= !!(cmdbyte & KBC_CMDBYTE_TRANSPSAUX);

    if(!enable_devices())
        LOG(Logging::KEYBOARD, Serial::get().writef("kb: %s() failed at %d\n", __func__, __LINE__));

    // default+disable Keyboard
    if(!write_keyboard_ack(KB_CMD_DISABLE_SCAN))
        LOG(Logging::KEYBOARD, Serial::get().writef("kb: %s() failed at %d\n", __func__, __LINE__));

    // switch to our scancode set
    if(!_scset1) {
        if(!(write_keyboard_ack(KB_CMD_GETSET_SCANCODE) && write_keyboard_ack(2))) {
            LOG(Logging::KEYBOARD, Serial::get().writef(
                    "kb: %s() failed at %d -- buggy keyboard?\n", __func__, __LINE__));
            _scset1 = true;
        }
    }

    // enable Keyboard
    if(!write_keyboard_ack(KB_CMD_ENABLE_SCAN))
        LOG(Logging::KEYBOARD, Serial::get().writef("kb: %s() failed at %d\n", __func__, __LINE__));

    if(_mouse_enabled) {
        // reset mouse, we enable data reporting later after the reset is completed
        if(!write_mouse_ack(MOUSE_CMD_RESET)) {
            LOG(Logging::KEYBOARD, Serial::get().writef(
                    "kb: %s() failed at %d\nDisabling mouse.\n", __func__, __LINE__));
            _mouse_enabled = false;
        }

        // wait until we got response from the mice
        if(_mouse_enabled && !wait_output_full())
            LOG(Logging::KEYBOARD, Serial::get().writef("kb: %s() failed at %d\n", __func__, __LINE__));

        // if mouse enabling failed, we have to enable the keyboard again (at least on some machines)
        if(!_mouse_enabled)
            write_keyboard_ack(KB_CMD_ENABLE_SCAN);
    }

    // enable receiving
    _flags = Keyboard::NUM;
    _mousestate = 0xfe;

    // consume pending characters
    Keyboard::Packet kb;
    while(read(kb))
        ;
    Mouse::Packet mouse;
    while(read(mouse))
        ;
}
