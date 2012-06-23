/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <utcb/UtcbFrame.h>
#include <cap/CapSpace.h>
#include <kobj/Pt.h>
#include <BitField.h>
#include <CPU.h>

namespace nul {

class Connection {
public:
	explicit Connection(const char *service) : _available(), _caps(connect(service)), _pts() {
	}
	~Connection() {
		for(size_t i = 0; i < Hip::MAX_CPUS; ++i)
			delete _pts[i];
		CapSpace::get().free(_caps,Hip::MAX_CPUS);
	}

	bool available_on(cpu_t cpu) const {
		return _available.is_set(cpu);
	}
	Pt *pt(cpu_t cpu) {
		assert(available_on(cpu));
		if(_pts[cpu] == 0)
			_pts[cpu] = new Pt(_caps + cpu);
		return _pts[cpu];
	}

private:
	capsel_t connect(const char *service) {
		// TODO we need a function to receive caps, right?
		UtcbFrame uf;
		ScopedCapSels caps(Hip::MAX_CPUS,Hip::MAX_CPUS);
		uf.set_receive_crd(Crd(caps.get(),Math::next_pow2_shift<uint>(Hip::MAX_CPUS),Crd::OBJ_ALL));
		uf << String(service);
		CPU::current().get_pt->call(uf);
		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw Exception(res);
		uf >> _available;
		return caps.release();
	}

	Connection(const Connection&);
	Connection& operator=(const Connection&);

	BitField<Hip::MAX_CPUS> _available;
	capsel_t _caps;
	Pt *_pts[Hip::MAX_CPUS];
};

}
