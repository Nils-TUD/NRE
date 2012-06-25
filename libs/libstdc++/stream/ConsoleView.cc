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
	Console::ReceivePacket *pk = 0;
	do {
		pk = _consumer.get();
		_consumer.next();
	}
	while(pk->flags & Keyboard::RELEASE);
	return pk->character;
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
