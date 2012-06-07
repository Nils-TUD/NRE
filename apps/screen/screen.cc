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
	ScreenSessionData(Service *s,capsel_t caps,Pt::portal_func func)
		: SessionData(s,caps,func), _id(++_next_id) {
	}

	int id() const {
		return _id;
	}
	Producer<int> *prod() {
		return _prod;
	}

	virtual void set_ds(DataSpace *ds) {
		SessionData::set_ds(ds);
		_prod = new Producer<int>(ds);
	}

private:
	static int _next_id;
	int _id;
	Producer<int> *_prod;
};

class ScreenService : public Service {
public:
	ScreenService() : Service("screen",portal_write) {
	}

private:
	virtual SessionData *create_session(capsel_t caps,Pt::portal_func func) {
		return new ScreenSessionData(this,caps,func);
	}
};

int ScreenSessionData::_next_id = 0;
static ScreenService *srv;

int main() {
	// TODO might be something else than 0x3f8
	Caps::allocate(CapRange(0x3F8,6,Caps::TYPE_IO | Caps::IO_A));
	Caps::allocate(CapRange(0xB9,Util::blockcount(80 * 25 * 2,ExecEnv::PAGE_SIZE),
			Caps::TYPE_MEM | Caps::MEM_RW,ExecEnv::PHYS_START_PAGE + 0xB9));

	Screen::get().clear();
	Serial::get().init();
	Log::get().writef("I am the screen service!!\n");

	srv = new ScreenService();
	const Hip& hip = Hip::get();
	for(Hip::cpu_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled())
			srv->provide_on(it->id());
	}
	srv->reg();

	int x = 0;
	while(1) {
		for(SessionIterator<ScreenSessionData> it = srv->sessions_begin<ScreenSessionData>(); it != srv->sessions_end<ScreenSessionData>(); ++it) {
			if(it->prod()) {
				if(it->prod()->produce(x))
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
		Log::get().writef("Request on cpu %u from %d: %u\n",Ec::current()->cpu(),c->id(),i);
		uf << E_SUCCESS;
	}
	catch(const Exception& e) {
		uf.clear();
		uf << e.code();
	}
}
