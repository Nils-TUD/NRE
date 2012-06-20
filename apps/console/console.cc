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
#include <dev/Keyboard.h>
#include <kobj/GlobalEc.h>
#include <kobj/Sc.h>
#include <RCU.h>

#include "VGA.h"

using namespace nul;

class ConsoleSessionData : public SessionData {
public:
	ConsoleSessionData(Service *s,size_t id,capsel_t caps,Pt::portal_func func)
		: SessionData(s,id,caps,func), _sm(), _ec(receiver,next_cpu()), _sc(),
		  _in_ds(), _out_ds(), _cons() {
		_ec.set_tls<capsel_t>(Ec::TLS_PARAM,caps);
	}
	virtual ~ConsoleSessionData() {
		delete _out_ds;
		delete _in_ds;
		delete _prod;
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
	Producer<Keyboard::Packet> *prod() {
		return _prod;
	}

protected:
	virtual void accept_ds(DataSpace *ds) {
		ScopedLock<UserSm> guard(&_sm);
		if(_out_ds == 0) {
			_out_ds = ds;
			_prod = new Producer<Keyboard::Packet>(ds,false,false);
		}
		else if(_in_ds == 0) {
			_in_ds = ds;
			_cons = new Consumer<Console::Packet>(ds,false);
			_sc = new Sc(&_ec,Qpd());
			_sc->start();
		}
		else
			throw ServiceException(E_ARGS_INVALID);
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

	UserSm _sm;
	GlobalEc _ec;
	Sc *_sc;
	DataSpace *_in_ds;
	DataSpace *_out_ds;
	Producer<Keyboard::Packet> *_prod;
	Consumer<Console::Packet> *_cons;
	static CPU::iterator _cpu_it;
};

class ConsoleService : public Service {
public:
	typedef SessionIterator<ConsoleSessionData> iterator;

	ConsoleService(const char *name) : Service(name), _active(sessions_begin()) {
	}

	iterator sessions_begin() {
		return Service::sessions_begin<ConsoleSessionData>();
	}
	iterator sessions_end() {
		return Service::sessions_end<ConsoleSessionData>();
	}

	ConsoleSessionData *active() {
		return _active != sessions_end() ? &*_active : 0;
	}
	bool handle_keyevent(const Keyboard::Packet &pk);

private:
	virtual SessionData *create_session(size_t id,capsel_t caps,Pt::portal_func func) {
		return new ConsoleSessionData(this,id,caps,func);
	}

	iterator _active;
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
		if(sess == srv->active())
			vga->put(*pk);
		cons->next();
	}
}

bool ConsoleService::handle_keyevent(const Keyboard::Packet &pk) {
	switch(pk.keycode) {
		case Keyboard::VK_LEFT:
			if(~pk.flags & Keyboard::RELEASE) {
				if(_active == sessions_begin())
					_active = sessions_end();
				--_active;
			}
			return true;
		case Keyboard::VK_RIGHT:
			if(~pk.flags & Keyboard::RELEASE) {
				++_active;
				if(_active == sessions_end())
					_active = sessions_begin();
			}
			return true;
	}
	return false;
}

int main() {
	vga = new VGA();
	srv = new ConsoleService("console");
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it)
		srv->provide_on(it->id);
	srv->reg();

	Connection con("keyboard");
	KeyboardSession kb(con);
	while(1) {
		Keyboard::Packet *pk = kb.consumer().get();
		if(!srv->handle_keyevent(*pk)) {
			if(srv->active())
				srv->active()->prod()->produce(*pk);
		}
		kb.consumer().next();
	}
	return 0;
}
