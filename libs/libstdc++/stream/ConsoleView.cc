/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <stream/ConsoleView.h>

using namespace nul;

void ConsoleView::flush() {
	if(_row == Console::ROWS - 1) {
		scroll();
		_row--;
	}
	_pk->len = _col;
	_producer.next();
	_pk = _producer.current();
	_pk->cmd = Console::WRITE;
	_pk->x = 0;
	_pk->y = ++_row;
	_col = 0;
}

char ConsoleView::read() {
	char c;
	while(1) {
		Console::ReceivePacket pk = receive();
		c = pk.character;
		if(c != '\0' && (~pk.flags & Keyboard::RELEASE))
			break;
	}
	return c;
}

void ConsoleView::write(char c) {
	if(c == '\r')
		_col = 0;
	else if(c == '\n' || _col == Console::BUF_SIZE - 1) {
		flush();
	}
	else {
		_pk->buffer[_col++] = c;
		_pk->buffer[_col++] = 0 << 4 | 4;
	}
}

void ConsoleView::scroll() {
	Console::SendPacket pk;
	pk.cmd = Console::SCROLL;
	_producer.produce(pk);
}
