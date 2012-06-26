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
#include <service/Connection.h>
#include <service/Session.h>
#include <service/Producer.h>
#include <mem/DataSpace.h>

namespace nul {

class Screen {
public:
	enum {
		VIEW_COUNT	= 8,
		COLS		= 80,
		ROWS		= 25
	};

	enum Command {
		PAINT,
		SCROLL,
		SETVIEW
	};

	struct Packet {
		Command cmd;
		uint id;
		union {
			struct {
				uint8_t x;
				uint8_t y;
				uint8_t character;
				uint8_t color;
			} paint;
			uint view;
		};
	};

private:
	Screen();
};

class ScreenSession : public Session {
	enum {
		DS_SIZE	= ExecEnv::PAGE_SIZE
	};

public:
	explicit ScreenSession(Connection &con) : Session(con),
			_ds(DS_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW), _producer(&_ds,true) {
		_ds.share(*this);
	}

	void paint(uint id,uint8_t x,uint8_t y,uint8_t character,uint8_t color) {
		Screen::Packet pk;
		pk.cmd = Screen::PAINT;
		pk.id = id;
		pk.paint.x = x;
		pk.paint.y = y;
		pk.paint.character = character;
		pk.paint.color = color;
		_producer.produce(pk);
	}

	void scroll(uint id) {
		Screen::Packet pk;
		pk.id = id;
		pk.cmd = Screen::SCROLL;
		_producer.produce(pk);
	}

	void set_view(uint id,uint view) {
		Screen::Packet pk;
		pk.cmd = Screen::SETVIEW;
		pk.id = id;
		pk.view = view;
		_producer.produce(pk);
	}

private:
	DataSpace _ds;
	Producer<Screen::Packet> _producer;
};

}
