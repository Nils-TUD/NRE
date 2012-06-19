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
#include <service/Consumer.h>
#include <dev/Console.h>
#include <kobj/GlobalEc.h>
#include <kobj/Sc.h>
#include <RCU.h>

#include "VGA.h"

using namespace nul;

class ConsoleSessionData : public SessionData {
public:
	ConsoleSessionData(Service *s,size_t id,capsel_t caps,Pt::portal_func func)
		: SessionData(s,id,caps,func), _ec(receiver,next_cpu()), _sc(), _cons() {
		_ec.set_tls<capsel_t>(Ec::TLS_PARAM,caps);
	}
	virtual ~ConsoleSessionData() {
		delete _cons;
		delete _sc;
	}

	virtual void invalidate() {
		if(_cons)
			_cons->stop();
	}

	Consumer<Console::Packet> *cons() {
		return _cons;
	}

protected:
	virtual void set_ds(DataSpace *ds) {
		SessionData::set_ds(ds);
		_cons = new Consumer<Console::Packet>(ds,false);
		_sc = new Sc(&_ec,Qpd());
		_sc->start();
	}

private:
	static cpu_t next_cpu() {
		static UserSm sm;
		ScopedLock<UserSm> guard(&sm);
		if(_cpu_it == CPU::end())
			_cpu_it = CPU::begin();
		return (_cpu_it++)->id;
	}

	static void receiver(void *);

	GlobalEc _ec;
	Sc *_sc;
	Consumer<Console::Packet> *_cons;
	static CPU::iterator _cpu_it;
};

class ConsoleService : public Service {
public:
	typedef SessionIterator<ConsoleSessionData> iterator;

	ConsoleService(const char *name) : Service(name) {
	}

	iterator sessions_begin() {
		return Service::sessions_begin<ConsoleSessionData>();
	}
	iterator sessions_end() {
		return Service::sessions_end<ConsoleSessionData >();
	}

private:
	virtual SessionData *create_session(size_t id,capsel_t caps,Pt::portal_func func) {
		return new ConsoleSessionData(this,id,caps,func);
	}
};

CPU::iterator ConsoleSessionData::_cpu_it;
static VGA *vga;
static ConsoleService *srv;

void ConsoleSessionData::receiver(void *) {
	capsel_t caps = Ec::current()->get_tls<word_t>(Ec::TLS_PARAM);
	while(1) {
		ScopedLock<RCULock> guard(&RCU::lock());
		ConsoleSessionData *sess = srv->get_session<ConsoleSessionData>(caps);
		Consumer<Console::Packet> *cons = sess->cons();
		Console::Packet *pk = cons->get();
		if(pk == 0)
			return;
		vga->put(*pk);
		cons->next();
	}
}

int main() {
	vga = new VGA();
	srv = new ConsoleService("console");
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it)
		srv->provide_on(it->id);
	srv->reg();
	srv->wait();
	return 0;
}
