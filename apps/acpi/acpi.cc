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

#include <service/Service.h>

using namespace nul;

#if 0
class ACPISessionData : public SessionData {
public:
	ACPISessionData(Service *s,size_t id,capsel_t caps,Pt::portal_func func)
		: SessionData(s,id,caps,func), _prod(), _ds() {
	}
	virtual ~ACPISessionData() {
		delete _ds;
		delete _prod;
	}

	Producer<T> *prod() {
		return _prod;
	}

protected:
	virtual void accept_ds(DataSpace *ds) {
		_ds = ds;
		_prod = new Producer<T>(ds,false,false);
	}

private:
	Producer<T> *_prod;
	DataSpace *_ds;
};

class ACPIService : public Service {
public:
	typedef SessionIterator<ACPISessionData<T> > iterator;

	ACPIService(const char *name,Pt::portal_func func) : Service(name,func) {
	}

private:
	virtual SessionData *create_session(size_t id,capsel_t caps,Pt::portal_func func) {
		return new ACPISessionData(this,id,caps,func);
	}
};

static HostACPI *hostacpi;

PORTAL static void portal_acpi(capsel_t pid) {
	UtcbFrameRef uf;
	try {
		String name;
		if(!hostacpi)
			throw Exception(E_NOT_FOUND);

		uf >> name;
		uf.finish_input();

		uintptr_t off = hostacpi->find(name.str());
		uf << E_SUCCESS << off;
	}
	catch(const Exception &e) {
		uf.clear();
		uf << e.code();
	}
}

int main() {
	try {
		hostacpi = new HostACPI();
	}
	catch(const Exception &e) {
		Serial::get() << e.name() << ": " << e.msg() << "\n";
	}

	ACPIService *srv = new ACPIService("acpi",portal_acpi);
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it)
		srv->provide_on(it->id);
	srv->reg();

	Sm sm;
	sm.down();
	return 0;
}
#endif

int main() { return 0; }
