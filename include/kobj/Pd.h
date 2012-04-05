/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <kobj/KObject.h>
#include <cap/CapSpace.h>
#include <cap/ResourceSpace.h>

class Utcb;
class Hip;

extern "C" int start(cpu_t cpu,Utcb *utcb,Hip *hip);

class Pd : public KObject {
	friend int start(cpu_t cpu,Utcb *utcb,Hip *hip);

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
