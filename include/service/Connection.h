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
#include <util/BitField.h>
#include <CPU.h>

namespace nul {

/**
 * Represents a connection to a service in the sense that we have portals to open and close sessions
 * at the service.
 */
class Connection {
public:
	/**
	 * Opens the connection to the service with given name
	 *
	 * @param service the service-name
	 * @throws Exception if the connection failed
	 */
	explicit Connection(const char *service)
		: _available(), _caps(connect(service)), _pts(new Pt*[CPU::count()]) {
		for(size_t i = 0; i < CPU::count(); ++i)
			_pts[i] = 0;
	}
	/**
	 * Closes the connection
	 */
	~Connection() {
		for(size_t i = 0; i < CPU::count(); ++i)
			delete _pts[i];
		delete[] _pts;
		CapSpace::get().free(_caps,CPU::count());
	}

	/**
	 * @param log_id the logical CPU id
	 * @return true if you can use the given CPU to talk to the service
	 */
	bool available_on(cpu_t log_id) const {
		return _available.is_set(log_id);
	}
	/**
	 * @param log_id the logical CPU id
	 * @return the portal for CPU <cpu>
	 */
	Pt *pt(cpu_t log_id) {
		assert(available_on(log_id));
		if(_pts[log_id] == 0)
			_pts[log_id] = new Pt(_caps + log_id);
		return _pts[log_id];
	}

private:
	capsel_t connect(const char *service) {
		UtcbFrame uf;
		ScopedCapSels caps(CPU::count(),CPU::count());
		uf.set_receive_crd(Crd(caps.get(),Math::next_pow2_shift<uint>(CPU::count()),Crd::OBJ_ALL));
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
	Pt **_pts;
};

}
