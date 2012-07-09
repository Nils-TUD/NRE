/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <stream/ConsoleStream.h>

using namespace nul;

char ConsoleStream::read() {
	char c;
	while(1) {
		Console::ReceivePacket pk = _sess.receive();
		c = pk.character;
		if(c != '\0' && (~pk.flags & Keyboard::RELEASE))
			break;
	}
	return c;
}

void ConsoleStream::put(ushort value,ushort *base,uint &pos) {
	bool visible = false;
	switch(value & 0xff) {
		// backspace
		case 8:
			if(pos)
				pos--;
			break;
		case '\n':
			pos += Console::COLS - (pos % Console::COLS);
			break;
		case '\r':
			pos -= pos % Console::COLS;
			break;
		case '\t':
			pos += Console::TAB_WIDTH - (pos % Console::TAB_WIDTH);
			break;
		default:
			visible = true;
			break;
	}

	// scroll?
	if(pos >= Console::COLS * Console::ROWS) {
		memmove(base,base + Console::COLS,(Console::ROWS - 1) * Console::COLS * 2);
		memset(base + (Console::ROWS - 1) * Console::COLS,0,Console::COLS * 2);
		pos = Console::COLS * (Console::ROWS - 1);
	}
	if(visible)
		base[pos++] = value;
}
