/*
 * TODO comment me
 *
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NUL.
 *
 * NUL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NUL is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <stream/OStream.h>

namespace nul {

class Serial : public OStream {
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
	static Serial& get() {
		return _inst;
	}

	void init();
	virtual void write(char c);

private:
	explicit Serial() : OStream(), _inited(false) {
	}

	bool _inited;
	static Serial _inst;
};

}
