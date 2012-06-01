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
#include <mem/DataSpace.h>
#include <ScopedLock.h>
#include <CPU.h>

using namespace nul;

PORTAL static void portal_write(capsel_t);

class ScreenClientData : public ClientData {
public:
	ScreenClientData(capsel_t srvpt,capsel_t dspt,Pt::portal_func func)
		: ClientData(srvpt,dspt,func), _id(++_next_id) {
	}

	int id() const {
		return _id;
	}

private:
	static int _next_id;
	int _id;
};

class ScreenService : public Service {
public:
	ScreenService() : Service("screen",portal_write) {
	}

private:
	virtual ClientData *create_client(capsel_t srvpt,capsel_t dspt,Pt::portal_func func) {
		return new ScreenClientData(srvpt,dspt,func);
	}
};

int ScreenClientData::_next_id = 0;
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
			srv->reg(it->id());
	}
	srv->wait();
	return 0;
}

static void portal_write(capsel_t pid) {
	static UserSm sm;
	ScopedLock<UserSm> guard(&sm);
	ScreenClientData *c = srv->get_client<ScreenClientData>(pid);
	UtcbFrameRef uf;
	int i;
	int *data = reinterpret_cast<int*>(c->ds()->virt());
	i = *data;
	//uf >> i;
	Log::get().writef("Request on cpu %u from %d: %u\n",Ec::current()->cpu(),c->id(),i);
}
