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

#include "util.h"
#include "ports.h"

class Keyb {
    enum {
        PORT_CTRL                   = 0x64,
        PORT_DATA                   = 0x60
    };

    enum {
        STATUS_DATA_AVAIL           = 1 << 0,
        STATUS_BUSY                 = 1 << 1,
        STATUS_SELFTEST_OK          = 1 << 2,
        STATUS_LAST_WAS_CMD         = 1 << 3,
        STATUS_KB_LOCKED            = 1 << 4,
        STATUS_MOUSE_DATA_AVAIL     = 1 << 5,
        STATUS_TIMEOUT              = 1 << 6,
        STATUS_PARITY_ERROR         = 1 << 7,
        ACK                         = 0xFA
    };

    enum {
        KBC_CMD_READ_STATUS         = 0x20,
        KBC_CMD_SET_STATUS          = 0x60,
        KBC_CMD_DISABLE_MOUSE       = 0xA7,
        KBC_CMD_ENABLE_MOUSE        = 0xA8,
        KBC_CMD_DISABLE_KEYBOARD    = 0xAD,
        KBC_CMD_ENABLE_KEYBOARD     = 0xAE,
        KBC_CMD_NEXT2MOUSE          = 0xD4,
    };

    enum {
        KBC_CMDBYTE_TRANSPSAUX      = 1 << 6,
        KBC_CMDBYTE_DISABLE_KB      = 1 << 4,
        KBC_CMDBYTE_IRQ2            = 1 << 1,
        KBC_CMDBYTE_IRQ1            = 1 << 0,
    };

    enum {
        KB_CMD_GETSET_SCANCODE      = 0xF0,
        KB_CMD_ENABLE_SCAN          = 0xF4,
        KB_CMD_DISABLE_SCAN         = 0xF5,
    };

public:
    static void init();

    static uint8_t read() {
        uint8_t status;
        if((status = Ports::in<uint8_t>(PORT_CTRL)) & STATUS_DATA_AVAIL) {
            if(~status & STATUS_MOUSE_DATA_AVAIL)
                return Ports::in<uint8_t>(PORT_DATA);
        }
        return 0;
    }

private:
    static bool wait_status(uint8_t mask, uint8_t value);
    static bool wait_ack();
    static bool wait_output_full();
    static bool wait_input_empty();
    static bool read_cmd(uint8_t cmd, uint8_t &value);
    static bool write_cmd(uint8_t cmd, uint8_t value);
    static bool enable_device();
};
