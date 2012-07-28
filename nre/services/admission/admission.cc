/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#include <kobj/Sc.h>
#include <ipc/Service.h>
#include <services/Admission.h>

using namespace nre;

class AdmissionServiceSession : public ServiceSession {
	enum {
		MIN_QUANTUM	= 10000,
		MAX_QUANTUM	= 100000,
		MIN_PRIO	= 1,
		MAX_PRIO	= 10,
	};

public:
	explicit AdmissionServiceSession(Service *s,size_t id,capsel_t caps,Pt::portal_func func)
		: ServiceSession(s,id,caps,func), _name(), _sc(), _ec() {
	}
	virtual ~AdmissionServiceSession() {
		delete _sc;
		delete _ec;
	}

	void start(const String &name,capsel_t ec,cpu_t cpu,Qpd &qpd) {
		if(_ec)
			throw Exception(E_EXISTS);
		_name = name;
		_ec = new Ec(ec,cpu);
		qpd = Qpd(Math::min<uint>(MAX_PRIO,Math::max<uint>(MIN_PRIO,qpd.prio())),
				Math::min<uint>(MAX_QUANTUM,Math::max<uint>(MIN_QUANTUM,qpd.quantum())));
		_sc = new nre::Sc(static_cast<GlobalThread*>(_ec),qpd);
		_sc->start();
	}

private:
	String _name;
	nre::Sc *_sc;
	Ec *_ec;
};

class AdmissionService : public Service {
public:
	explicit AdmissionService(const char *name)
		: Service(name,CPUSet(CPUSet::ALL),portal) {
		// we want to accept one Ec
		for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it) {
			LocalThread *ec = get_thread(it->log_id());
			UtcbFrameRef uf(ec->utcb());
			uf.accept_delegates(0);
		}
	}

private:
	PORTAL static void portal(capsel_t pid);

	virtual ServiceSession *create_session(size_t id,capsel_t caps,Pt::portal_func func) {
		return new AdmissionServiceSession(this,id,caps,func);
	}
};

static AdmissionService *srv;

void AdmissionService::portal(capsel_t pid) {
	UtcbFrameRef uf;
	try {
		ScopedLock<RCULock> guard(&RCU::lock());
		AdmissionServiceSession *sess = srv->get_session<AdmissionServiceSession>(pid);
		Admission::Command cmd;
		uf >> cmd;

		switch(cmd) {
			case Admission::START: {
				String name;
				Qpd qpd;
				cpu_t cpu;
				capsel_t cap = uf.get_delegated(0).offset();
				uf >> name >> cpu >> qpd;
				uf.finish_input();

				sess->start(name,cap,cpu,qpd);
				uf.accept_delegates();
				uf << E_SUCCESS << qpd;
			}
			break;
		}
	}
	catch(const Exception &e) {
		uf.clear();
		uf << e.code();
	}
}

int main() {
	srv = new AdmissionService("admission");
	Sm sm(0);
	sm.down();
	return 0;
}
