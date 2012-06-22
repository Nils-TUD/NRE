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
#include <service/Connection.h>
#include <service/Session.h>
#include <dev/Console.h>
#include <cstring>

namespace nul {

class ConsoleView : public IStream, public OStream {
public:
	ConsoleView(ConsoleSession &sess,uint view)
		: _sess(sess), _view(view), _col(0), _row(0) {
	}

	virtual char read();
	virtual void write(char c);

private:
	void scroll();

	ConsoleSession &_sess;
	uint _view;
	uint8_t _col;
	uint8_t _row;
};

}
