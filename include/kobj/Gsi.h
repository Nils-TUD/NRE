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
#include <cap/CapHolder.h>
#include <Syscalls.h>
#include <CPU.h>

namespace nul {

class Gsi : public Sm {
public:
	enum Op {
		ALLOC,
		RELEASE
	};

	Gsi(uint gsi) : Sm(alloc(gsi),true), _gsi(gsi) {
		// neither keep the cap nor the selector
		set_flags(0);
	}
	virtual ~Gsi() {
		release();
	}

	uint gsi() const {
		return _gsi;
	}

private:
	capsel_t alloc(uint gsi) {
		UtcbFrame uf;
		CapHolder cap;
		uf.set_receive_crd(Crd(cap.get(),0,Crd::OBJ_ALL));
		uf << ALLOC << gsi;
		CPU::current().gsi_pt->call(uf);
		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw Exception(res);

		Syscalls::assign_gsi(cap.get(),CPU::current().id);
		return cap.release();
	}
	void release() {
		UtcbFrame uf;
		uf << RELEASE << _gsi;
		CPU::current().gsi_pt->call(uf);
	}

	uint _gsi;
};

}
