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
	else if(c == '\n' || _col == Console::COLS) {
		_row++;
		write('\r');
	}
	else {
		if(_row == Console::ROWS) {
			scroll();
			_row--;
		}
		Console::SendPacket pk;
		pk.cmd = Console::WRITE;
		pk.character = c;
		pk.color = 0 << 4 | 4;
		pk.x = _col++;
		pk.y = _row;
		_producer.produce(pk);
	}
}

void ConsoleView::scroll() {
	Console::SendPacket pk;
	pk.cmd = Console::SCROLL;
	_producer.produce(pk);
}
