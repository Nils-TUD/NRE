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
#include <kobj/Pt.h>
#include <ipc/ClientSession.h>
#include <ipc/Connection.h>
#include <services/Keyboard.h>
#include <Hip.h>

namespace nre {

/**
 * Types for the console service
 */
class Console {
public:
	/**
	 * Basic attributes
	 */
	enum {
		COLS		= 80,
		ROWS		= 25,
		TAB_WIDTH	= 4,
		BUF_SIZE	= COLS * 2 + 1,
		PAGES		= 32,
		TEXT_OFF	= 0x18000,
		TEXT_PAGES	= 8,
		PAGE_SIZE	= 0x1000,
		SUBCONS		= 32,
	};

	/**
	 * The available commands
	 */
	enum Command {
		CREATE,
		SET_REGS
	};

	/**
	 * Specifies attributes for the console
	 */
	struct Register {
		uint16_t mode;
		uint16_t cursor_style;
		uint32_t cursor_pos;
		size_t offset;
	};

	/**
	 * A packet that we receive from the console
	 */
	struct ReceivePacket {
		uint flags;
		uint8_t scancode;
		uint8_t keycode;
		char character;
	};
};

/**
 * Represents a session at the console service
 */
class ConsoleSession : public ClientSession {
	enum {
		IN_DS_SIZE	= ExecEnv::PAGE_SIZE,
		OUT_DS_SIZE	= ExecEnv::PAGE_SIZE * Console::PAGES
	};

public:
	/**
	 * Creates a new session with given connection. That is, it creates a new subconsole attached
	 * to the given console.
	 *
	 * @param con the connection
	 * @param console the console to attach to
	 * @param title the subconsole title
	 */
	explicit ConsoleSession(Connection &con,size_t console,const String &title)
			: ClientSession(con), _in_ds(IN_DS_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW),
			  _out_ds(OUT_DS_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW), _consumer(&_in_ds,true) {
		create(console,title);
	}

	/**
	 * @return the screen memory (might be directly mapped or buffered)
	 */
	const DataSpace &screen() const {
		return _out_ds;
	}

	/**
	 * Clears the given page
	 *
	 * @param page the page
	 */
	void clear(uint page) {
		assert(page < Console::TEXT_PAGES);
		uintptr_t addr = screen().virt() + Console::TEXT_OFF + page * Console::PAGE_SIZE;
		memset(reinterpret_cast<void*>(addr),	0,Console::PAGE_SIZE);
	}

	/**
	 * Sets the given registers
	 *
	 * @param regs the registers
	 */
	void set_regs(const Console::Register &regs) {
		UtcbFrame uf;
		uf << Console::SET_REGS << regs;
		Pt pt(caps() + CPU::current().log_id());
		pt.call(uf);
		uf.check_reply();
	}

	/**
	 * @return the consumer to receive packets from the console
	 */
	Consumer<Console::ReceivePacket> &consumer() {
		return _consumer;
	}

	/**
	 * Receives the next packet from the console. I.e. it waits until the next packet arrives.
	 *
	 * @return the received packet
	 * @throws Exception if it failed
	 */
	Console::ReceivePacket receive() {
		Console::ReceivePacket *pk = _consumer.get();
		if(!pk)
			throw Exception(E_ABORT,"Unable to receive console packet");
		Console::ReceivePacket res = *pk;
		_consumer.next();
		return res;
	}

private:
	void create(size_t console,const String &title) {
		UtcbFrame uf;
		uf << Console::CREATE << console << title;
		uf.delegate(_in_ds.sel(),0);
		uf.delegate(_out_ds.sel(),1);
		Pt pt(caps() + CPU::current().log_id());
		pt.call(uf);
		uf.check_reply();
	}

	DataSpace _in_ds;
	DataSpace _out_ds;
	Consumer<Console::ReceivePacket> _consumer;
};

}
