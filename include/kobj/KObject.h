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

#include <Types.h>

namespace nul {

class KObject {
private:
	enum {
		KEEP_BIT	= 1 << (sizeof(cap_t) * 8 - 1)
	};

public:
	enum {
		INVALID 	= (cap_t)-1
	};

	virtual ~KObject();

	cap_t cap() const {
		return _cap & ~KEEP_BIT;
	}

protected:
	KObject(cap_t cap = INVALID) : _cap(cap | KEEP_BIT) {
	}
	void cap(cap_t cap,bool keep = false) {
		_cap = cap;
		if(keep)
			_cap |= KEEP_BIT;
	}

private:
	KObject(const KObject&);
	KObject& operator=(const KObject&);

	cap_t _cap;
};

}
