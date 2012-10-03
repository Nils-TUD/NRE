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

#pragma once

#include <arch/Types.h>
#include <kobj/Ports.h>
#include <util/Clock.h>
#include <services/Keyboard.h>
#include <services/Mouse.h>

/**
 * A PS/2 host keyboard and mouse driver.  Translates SCS2 keycodes to
 * single extended keycode and mouse movements to mouse packets.  Both
 * are forwarded on the keycode bus.
 *
 * State: stable
 * Features: scancode set1+2, simple PS2 mouse incl. wheel-support
 * Documentation: PS2 hitrc chapter 7+11, scancodes-13.html
 */
class HostKeyboard {
	static const uint FREQ			= 1000;
	static const uint TIMEOUT		= 50;
	static const uint PORT_BASE	= 0x60;

	enum {
		STATUS_DATA_AVAIL			= 1 << 0,
		STATUS_BUSY					= 1 << 1,
		STATUS_SELFTEST_OK			= 1 << 2,
		STATUS_LAST_WAS_CMD			= 1 << 3,
		STATUS_KB_LOCKED			= 1 << 4,
		STATUS_MOUSE_DATA_AVAIL		= 1 << 5,
		STATUS_TIMEOUT				= 1 << 6,
		STATUS_PARITY_ERROR			= 1 << 7,
		ACK							= 0xFA
	};

	enum {
		KBC_CMD_READ_STATUS			= 0x20,
		KBC_CMD_SET_STATUS			= 0x60,
		KBC_CMD_DISABLE_MOUSE		= 0xA7,
		KBC_CMD_ENABLE_MOUSE		= 0xA8,
		KBC_CMD_DISABLE_KEYBOARD	= 0xAD,
		KBC_CMD_ENABLE_KEYBOARD		= 0xAE,
		KBC_CMD_NEXT2MOUSE			= 0xD4,
	};

	enum {
		KBC_CMDBYTE_TRANSPSAUX		= 1 << 6,
		KBC_CMDBYTE_DISABLE_KB		= 1 << 4,
		KBC_CMDBYTE_IRQ2			= 1 << 1,
		KBC_CMDBYTE_IRQ1			= 1 << 0,
	};

	enum {
		KB_CMD_GETSET_SCANCODE		= 0xF0,
		KB_CMD_ENABLE_SCAN			= 0xF4,
		KB_CMD_DISABLE_SCAN			= 0xF5,
	};

	enum {
		MOUSE_CMD_SETSAMPLE			= 0xF3,
		MOUSE_CMD_STREAMING			= 0xF4,
		MOUSE_CMD_GETDEVID			= 0xF2,
		MOUSE_CMD_RESET				= 0xFF,
	};

public:
	struct ScanCodeEntry {
		uint8_t def;
		uint8_t ext0;
		uint8_t ext1;
	};

	explicit HostKeyboard(int scset = 2,bool mouse = false)
		: _clock(FREQ), _port_ctrl(PORT_BASE + 4,1), _port_data(PORT_BASE,1), _flags(0), _mousestate(),
		  _mouse_enabled(mouse), _wheel(false), _scset1(scset == 1) {
	}

	bool mouse_enabled() const {
		return _mouse_enabled;
	}
	bool read(nre::Keyboard::Packet &data);
	bool read(nre::Mouse::Packet &data);
	void reboot();
	void reset();

private:
	bool wait_ack();
	bool wait_status(uint8_t mask,uint8_t value);
	bool wait_output_full();
	bool wait_input_empty();

	bool disable_devices();
	bool enable_devices();
	void enable_mouse();

	bool read_cmd(uint8_t cmd,uint8_t &value);
	bool write_cmd(uint8_t cmd,uint8_t value);

	bool write_keyboard_ack(uint8_t value);
	bool write_mouse_ack(uint8_t value);

	bool handle_aux(nre::Mouse::Packet &data,uint8_t byte);
	bool handle_scancode(nre::Keyboard::Packet &data,uint8_t key);

	static uint8_t sc1_to_sc2(uint8_t scancode);

	nre::Clock _clock;
	nre::Ports _port_ctrl;
	nre::Ports _port_data;
	uint _flags;
	uint8_t _mousestate;
	bool _mouse_enabled;
	bool _wheel;
	bool _scset1;
	static ScanCodeEntry scset2[];
};
