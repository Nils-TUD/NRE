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

#include <kobj/KObject.h>
#include <cap/CapSpace.h>
#include <cap/ResourceSpace.h>

class Utcb;
class Hip;

extern "C" int start(cpu_t cpu,Utcb *utcb);

class Pd : public KObject {
	friend int start(cpu_t cpu,Utcb *utcb);

public:
	static Pd *current();

private:
	explicit Pd(cap_t cap,bool) : KObject(this,cap), _io(0,0), _mem(0,0), _obj() {
	}
public:
	explicit Pd(cap_t cap) : KObject(current(),cap), _io(0,0), _mem(0,0), _obj() {
	}

	ResourceSpace &io() {
		return _io;
	}
	ResourceSpace &mem() {
		return _mem;
	}
	CapSpace &obj() {
		return _obj;
	}

private:
	Pd(const Pd&);
	Pd& operator=(const Pd&);

private:
	ResourceSpace _io;
	ResourceSpace _mem;
	CapSpace _obj;
};
