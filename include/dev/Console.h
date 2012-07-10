/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <arch/Types.h>
#include <kobj/Pt.h>
#include <service/Session.h>
#include <service/Connection.h>
#include <dev/Keyboard.h>
#include <Hip.h>

namespace nul {

class Console {
public:
	enum {
		COLS		= 80,
		ROWS		= 25,
		TAB_WIDTH	= 4,
		BUF_SIZE	= COLS * 2 + 1,
		PAGES		= 32,
		TEXT_OFF	= 0x18000,
		TEXT_PAGES	= 8,
		PAGE_SIZE	= 0x1000
	};

	enum Command {
		CREATE,
		SET_REGS
	};

	struct Register {
		uint16_t mode;
		uint16_t cursor_style;
		uint32_t cursor_pos;
		uintptr_t offset;
	};

	struct ReceivePacket {
		uint flags;
		uint8_t scancode;
		uint8_t keycode;
		char character;
	};
};

class ConsoleSession : public Session {
	enum {
		IN_DS_SIZE	= ExecEnv::PAGE_SIZE,
		OUT_DS_SIZE	= ExecEnv::PAGE_SIZE * Console::PAGES
	};

public:
	explicit ConsoleSession(Connection &con,bool show_pages = true)
			: Session(con), _in_ds(IN_DS_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW),
			  _out_ds(OUT_DS_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW), _consumer(&_in_ds,true) {
		create(show_pages);
	}

	const DataSpace &screen() const {
		return _out_ds;
	}

	void set_regs(const Console::Register &regs) {
		UtcbFrame uf;
		uf << Console::SET_REGS << regs;
		Pt pt(caps() + CPU::current().log_id());
		pt.call(uf);
		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw Exception(res);
	}

	Console::ReceivePacket receive() {
		Console::ReceivePacket *pk = _consumer.get();
		if(!pk)
			throw Exception(E_ABORT);
		Console::ReceivePacket res = *pk;
		_consumer.next();
		return res;
	}

private:
	void create(bool show_pages) {
		UtcbFrame uf;
		uf << Console::CREATE << _in_ds.desc() << _out_ds.desc() << show_pages;
		uf.delegate(_in_ds.sel(),0);
		uf.delegate(_out_ds.sel(),1);
		Pt pt(caps() + CPU::current().log_id());
		pt.call(uf);
		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw Exception(res);
	}

	DataSpace _in_ds;
	DataSpace _out_ds;
	Consumer<Console::ReceivePacket> _consumer;
};

}
