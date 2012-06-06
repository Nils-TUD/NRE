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

#include <utcb/UtcbFrame.h>
#include <service/Service.h>
#include <cap/CapSpace.h>
#include <kobj/Pt.h>
#include <CPU.h>

namespace nul {

class Session {
public:
	Session(const char *service) : _pts(), _pt(connect(service)) {
		get_portals();
	}
	~Session() {
		for(size_t i = 0; i < Hip::MAX_CPUS; ++i)
			delete _pts[i];
		CapSpace::get().free(_caps,Hip::MAX_CPUS);
	}

	Pt &session_pt() {
		return _pt;
	}
	bool available_on(cpu_t cpu) const {
		return _pts[cpu] != 0;
	}
	Pt &pt(cpu_t cpu) {
		assert(available_on(cpu));
		return *_pts[cpu];
	}

private:
	capsel_t connect(const char *service) const {
		// TODO we need a function to receive caps, right?
		UtcbFrame uf;
		CapHolder caps(Hip::MAX_CPUS,Hip::MAX_CPUS);
		uf.set_receive_crd(Crd(caps.get(),Util::nextpow2shift<uint>(Hip::MAX_CPUS),DESC_CAP_ALL));
		uf << String(service);
		CPU::current().get_pt->call(uf);
		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw Exception(res);
		return caps.release() + CPU::current().id;
	}

	void get_portals() {
		UtcbFrame uf;
		CapHolder caps(Hip::MAX_CPUS,Hip::MAX_CPUS);
		uf.set_receive_crd(Crd(caps.get(),Util::nextpow2shift<uint>(Hip::MAX_CPUS),DESC_CAP_ALL));
		uf << Service::OPEN_SESSION;
		_pt.call(uf);

		BitField<Hip::MAX_CPUS> bf;
		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw Exception(res);

		uf >> bf;
		for(size_t i = 0; i < Hip::MAX_CPUS; ++i) {
			if(bf.is_set(i))
				_pts[i] = new Pt(caps.get() + i);
		}
		_caps = caps.release();
	}

	Session(const Session&);
	Session& operator=(const Session&);

	capsel_t _caps;
	Pt *_pts[Hip::MAX_CPUS];
	Pt _pt;
};

}
