/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <kobj/Sm.h>
#include <kobj/Pt.h>
#include <utcb/UtcbFrame.h>
#include <util/ScopedCapSels.h>
#include <Syscalls.h>
#include <CPU.h>

namespace nul {

/**
 * Represents a global system interrupt. Creation of a GSI allocates it from the parent and
 * destruction release it again. By doing a down() or zero() on this object you can wait for
 * the interrupt.
 */
class Gsi : public Sm {
public:
	enum Op {
		ALLOC,
		RELEASE
	};

	/**
	 * Allocates the given GSI from the parent
	 *
	 * @param gsi the GSI
	 */
	explicit Gsi(uint gsi) : Sm(alloc(gsi),true), _gsi(gsi) {
		// neither keep the cap nor the selector
		set_flags(0);
	}
	/**
	 * Releases the GSI
	 */
	virtual ~Gsi() {
		release();
	}

	/**
	 * @return the GSI
	 */
	uint gsi() const {
		return _gsi;
	}

private:
	capsel_t alloc(uint gsi) {
		UtcbFrame uf;
		ScopedCapSels cap;
		uf.set_receive_crd(Crd(cap.get(),0,Crd::OBJ_ALL));
		uf << ALLOC << gsi;
		CPU::current().gsi_pt->call(uf);
		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw Exception(res);

		Syscalls::assign_gsi(cap.get(),CPU::current().phys_id());
		return cap.release();
	}
	void release() {
		try {
			UtcbFrame uf;
			uf << RELEASE << _gsi;
			CPU::current().gsi_pt->call(uf);
		}
		catch(...) {
			// ignore
		}
	}

	uint _gsi;
};

}
