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
		_col = 0;
		if(_row == Console::ROWS) {
			memmove(screen(),screen() + Console::COLS * 2,(Console::ROWS - 1) * Console::COLS * 2);
			memset(screen() + (Console::ROWS - 1) * Console::COLS * 2,0,Console::COLS * 2);
			_row--;
		}
	}
	else {
		char *pos = screen() + _row * Console::COLS * 2 + _col * 2;
		pos[0] = c;
		pos[1] = 0 << 4 | 4;
		_col++;
	}
}
