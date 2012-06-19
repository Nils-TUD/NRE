/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <kobj/Pt.h>
#include <kobj/Sm.h>
#include <kobj/UserSm.h>
#include <kobj/Gsi.h>
#include <kobj/Ports.h>
#include <cap/CapSpace.h>
#include <cap/Caps.h>
#include <stream/Screen.h>
#include <stream/Serial.h>
#include <stream/Log.h>
#include <service/Service.h>
#include <service/ServiceInstance.h>
#include <service/Producer.h>
#include <mem/DataSpace.h>
#include <ScopedLock.h>
#include <CPU.h>

using namespace nul;

PORTAL static void portal_write(capsel_t);

class ScreenSessionData : public SessionData {
public:
	ScreenSessionData(Service *s,size_t id,capsel_t caps,Pt::portal_func func)
		: SessionData(s,id,caps,func), _prod() {
	}

	Producer<int> *prod() {
		return _prod;
	}

	virtual void set_ds(DataSpace *ds) {
		SessionData::set_ds(ds);
		_prod = new Producer<int>(ds);
	}

private:
	Producer<int> *_prod;
};

class ScreenService : public Service {
public:
	ScreenService() : Service("screen",portal_write) {
	}

	SessionIterator<ScreenSessionData> sessions_begin() {
		return Service::sessions_begin<ScreenSessionData>();
	}
	SessionIterator<ScreenSessionData> sessions_end() {
		return Service::sessions_end<ScreenSessionData>();
	}

private:
	virtual SessionData *create_session(size_t id,capsel_t caps,Pt::portal_func func) {
		return new ScreenSessionData(this,id,caps,func);
	}
};

static ScreenService *srv;

int main() {
	Serial::get().writef("I am the screen service!!\n");

	{
		Gsi kb(1);
		for(int i = 0; i < 10; ++i) {
			kb.down();
			Serial::get().writef("Got keyboard interrupt\n");
		}
	}

	srv = new ScreenService();
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it)
		srv->provide_on(it->id);
	srv->reg();

	while(1) {
		Serial::get().writef("I am the screen service again!!\n");
	}

	int x = 0;
	while(1) {
		ScopedLock<RCULock> guard(&RCU::lock());
		for(SessionIterator<ScreenSessionData> it = srv->sessions_begin(); it != srv->sessions_end(); ++it) {
			if(it->prod()) {
				it->prod()->produce(x);
				x++;
			}
		}
	}

	srv->wait();
	return 0;
}

static void portal_write(capsel_t pid) {
	static UserSm sm;
	ScopedLock<UserSm> guard(&sm);
	UtcbFrameRef uf;
	try {
		ScreenSessionData *c = srv->get_session<ScreenSessionData>(pid);
		int *data = reinterpret_cast<int*>(c->ds()->virt());
		int i = *data;
		//uf >> i;
		Serial::get().writef("Request on cpu %u from %d: %u\n",Ec::current()->cpu(),c->id(),i);
		uf << E_SUCCESS;
	}
	catch(const Exception& e) {
		uf.clear();
		uf << e.code();
	}
}
