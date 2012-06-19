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
#include <service/Connection.h>
#include <service/Service.h>
#include <cap/CapSpace.h>
#include <kobj/Pt.h>
#include <CPU.h>

namespace nul {

class Session {
public:
	Session(Connection &con) : _caps(open(con)), _con(con) {
	}
	virtual ~Session() {
		close();
		CapSpace::get().free(_caps,Hip::MAX_CPUS);
	}

	Connection &con() {
		return _con;
	}
	capsel_t caps() const {
		return _caps;
	}

private:
	capsel_t open(Connection &con) {
		UtcbFrame uf;
		CapHolder caps(Hip::MAX_CPUS,Hip::MAX_CPUS);
		uf.set_receive_crd(Crd(caps.get(),Math::next_pow2_shift<uint>(Hip::MAX_CPUS),Crd::OBJ_ALL));
		uf << Service::OPEN_SESSION;
		con.pt(CPU::current().id)->call(uf);

		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw Exception(res);
		return caps.release();
	}

	void close() {
		UtcbFrame uf;
		uf.translate(_caps);
		uf << Service::CLOSE_SESSION;
		_con.pt(CPU::current().id)->call(uf);
	}

	Session(const Session&);
	Session& operator=(const Session&);

	capsel_t _caps;
	Connection &_con;
};

}
