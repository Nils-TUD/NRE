/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

#include <Types.h>

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

private:
	Pd *_pd;
	cap_t _cap;
};
