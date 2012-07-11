/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <stream/OStream.h>
#include <stream/IStream.h>
#include <ipc/Connection.h>
#include <ipc/Session.h>
#include <services/Console.h>
#include <cstring>

namespace nre {

class ConsoleStream : public IStream, public OStream {
public:
	ConsoleStream(ConsoleSession &sess,uint page = 0) : _sess(sess), _page(page), _pos(0) {
	}

	uint page() const {
		return _page;
	}

	virtual char read();
	virtual void write(char c) {
		put(0x0F00 | c,_pos);
	}
	void put(ushort value,uint &pos) {
		uintptr_t addr = _sess.screen().virt() + Console::TEXT_OFF + _page * Console::PAGE_SIZE;
		put(value,reinterpret_cast<ushort*>(addr),pos);
	}
	void put(ushort value,ushort *base,uint &pos);

private:
	ConsoleSession &_sess;
	uint _page;
	uint _pos;
};

}
