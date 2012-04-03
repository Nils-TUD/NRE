/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <Format.h>

class Log : public Format {
private:
	enum {
		COM1	= 0x3F8,
		COM2	= 0x2E8,
		COM3	= 0x2F8,
		COM4	= 0x3E8
	};
	enum {
		port = COM1
	};

public:
	Log();
	virtual ~Log() {
	}

protected:
	virtual void printc(char c);
};
