/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <service/Service.h>
#include <stream/Serial.h>
#include <kobj/Ports.h>
#include <kobj/Sm.h>

class Log : public nul::BaseSerial {
	enum {
		COM1	= 0x3F8,
		COM2	= 0x2E8,
		COM3	= 0x2F8,
		COM4	= 0x3E8
	};
	enum {
		DLR_LO	= 0,
		DLR_HI	= 1,
		IER		= 1,	// interrupt enable register
		FCR		= 2,	// FIFO control register
		LCR		= 3,	// line control register
		MCR		= 4,	// modem control register
	};
	enum {
		PORT_BASE		= COM1,
		ROOT_SESS		= 0
	};

public:
	static Log &get() {
		return _inst;
	}
	void reg();

private:
	explicit Log();

	void write(uint sessid,const char *line,size_t len);

	void out(char c);
	void out(const char *str) {
		while(*str)
			out(*str++);
	}

	void buffer(char c) {
		if(_bufpos == sizeof(_buf) || c == '\n') {
			write(ROOT_SESS,_buf,_bufpos);
			_bufpos = 0;
		}
		if(c != '\n')
			_buf[_bufpos++] = c;
	}

	virtual void write(char c) {
		if(c == '\0')
			return;

		if(_to_ser)
			out(c);
		else
			buffer(c);
	}

	PORTAL static void portal(capsel_t pid);

	nul::Ports _ports;
	nul::Sm _sm;
	bool _to_ser;
	size_t _bufpos;
	char _buf[MAX_LINE_LEN + 1];
	static Log _inst;
	static nul::Service *_srv;
	static const char *_colors[];
};
