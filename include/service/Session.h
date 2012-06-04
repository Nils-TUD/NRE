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
	Session(const char *service) : _pt(connect(service)) {
	}

	Pt &pt() {
		return _pt;
	}

private:
	static capsel_t connect(const char *service) {
		// TODO I think, a better solution would be to have a kind of boot-process that loads the
		// services and clients in a way that a service is always available when someone wants
		// to use it. this way, we don't need such a complicated and error-prone mechanism.

		// the idea is the following: the parent creates a semaphore for each client and cpu and
		// delegates them to the clients. whenever a client fails to connect to a service, the
		// parent stores that (for the cpu it used). when a service is registered on a specific cpu,
		// the parent goes through all clients and does an up on each semaphore for that cpu X times,
		// where X is the number of times a client tried to connect on that cpu.
		// note that we track the number of tries because a client might start multiple Ecs on one
		// cpu that try to connect to services, so that the parent can't simply notify the client
		// once for that cpu.
		capsel_t cap = ObjCap::INVALID;
		do {
			try {
				// TODO we need a function to receive a cap, right?
				UtcbFrame uf;
				uf.set_receive_crd(Crd(CapSpace::get().allocate(),0,DESC_CAP_ALL));
				uf << String(service);
				CPU::current().get_pt->call(uf);
				ErrorCode res;
				uf >> res;
				if(res != E_SUCCESS)
					throw Exception(res);

				TypedItem ti;
				uf.get_typed(ti);
				cap = ti.crd().cap();
			}
			catch(const Exception& e) {
				// if it failed, wait until the parent notifies us. down() instead of zero() is
				// correct here, because the parent tracks the number of tries and thus, he will
				// not do more ups than necessary. and it might happen that the connect failed,
				// but immediatly afterwards the service is registered and up() is called. when
				// calling zero() we would ignore this signal and might block for ever.
				Serial::get().writef("Waiting for service...\n");
				CPU::current().srv_sm->down();
				Serial::get().writef("Retrying...\n");
			}
		}
		while(cap == ObjCap::INVALID);
		return cap;
	}

	Session(const Session&);
	Session& operator=(const Session&);

	Pt _pt;
};

}
