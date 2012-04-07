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

class Pd;

class KObject {
protected:
	enum {
		INVALID = (cap_t)-1
	};

	KObject(Pd *pd,cap_t cap = INVALID) : _pd(pd), _cap(cap) {
	}
public:
	virtual ~KObject();

	Pd *pd() {
		return _pd;
	}
	cap_t cap() const {
		return _cap;
	}
protected:
	void cap(cap_t cap) {
		_cap = cap;
	}

private:
	KObject(const KObject&);
	KObject& operator=(const KObject&);

	Pd *_pd;
	cap_t _cap;
};

}
