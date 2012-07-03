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
#include <service/Producer.h>
#include <kobj/GlobalEc.h>
#include <kobj/Sc.h>
#include <kobj/Gsi.h>
#include <RCU.h>

#include "HostKeyboard.h"

using namespace nul;

template<class T>
class KeyboardSessionData : public SessionData {
public:
	KeyboardSessionData(Service *s,size_t id,capsel_t caps,Pt::portal_func func)
		: SessionData(s,id,caps,func), _prod(), _ds() {
	}
	virtual ~KeyboardSessionData() {
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

template<class T>
class KeyboardService : public Service {
public:
	typedef SessionIterator<KeyboardSessionData<T> > iterator;

	KeyboardService(const char *name,Pt::portal_func func = 0) : Service(name,func) {
	}

	iterator sessions_begin() {
		return Service::sessions_begin<KeyboardSessionData<T> >();
	}
	iterator sessions_end() {
		return Service::sessions_end<KeyboardSessionData<T> >();
	}

private:
	virtual SessionData *create_session(size_t id,capsel_t caps,Pt::portal_func func) {
		return new KeyboardSessionData<T>(this,id,caps,func);
	}
};

static HostKeyboard *hostkb;
static KeyboardService<Keyboard::Packet> *kbsrv;
static KeyboardService<Mouse::Packet> *mousesrv;

template<class T>
static void broadcast(KeyboardService<T> *srv,const T &data) {
	ScopedLock<RCULock> guard(&RCU::lock());
	for(typename KeyboardService<T>::iterator it = srv->sessions_begin(); it != srv->sessions_end(); ++it) {
		if(it->prod())
			it->prod()->produce(data);
	}
}

static void kbhandler(void*) {
	Gsi gsi(1);
	while(1) {
		gsi.down();

		Keyboard::Packet data;
		if(hostkb->read(data))
			broadcast(kbsrv,data);
	}
}

static void mousehandler(void*) {
	Gsi gsi(12);
	while(1) {
		gsi.down();

		Mouse::Packet data;
		if(hostkb->read(data))
			broadcast(mousesrv,data);
	}
}

PORTAL static void portal_keyboard(capsel_t) {
	UtcbFrameRef uf;
	try {
		Keyboard::Command cmd;
		uf >> cmd;
		switch(cmd) {
			case Keyboard::REBOOT:
				hostkb->reboot();
				break;
		}
		uf << E_SUCCESS;
	}
	catch(const Exception &e) {
		uf.clear();
		uf << e.code();
	}
}

int main() {
	hostkb = new HostKeyboard(0x60,2,true);
	hostkb->reset();

	kbsrv = new KeyboardService<Keyboard::Packet>("keyboard",portal_keyboard);
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it)
		kbsrv->provide_on(it->log_id());
	kbsrv->reg();

	if(hostkb->mouse_enabled()) {
		mousesrv = new KeyboardService<Mouse::Packet>("mouse");
		for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it)
			mousesrv->provide_on(it->log_id());
		mousesrv->reg();

		Sc *sc = new Sc(new GlobalEc(mousehandler,CPU::current().log_id()),Qpd());
		sc->start();
	}

	kbhandler(0);
	return 0;
}
