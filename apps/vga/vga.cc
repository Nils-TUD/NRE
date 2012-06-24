/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <kobj/UserSm.h>
#include <kobj/GlobalEc.h>
#include <kobj/Sc.h>
#include <service/Service.h>
#include <service/Consumer.h>
#include <mem/DataSpace.h>
#include <dev/Screen.h>
#include <ScopedLock.h>

#include "HostVGA.h"

using namespace nul;

class VGASessionData : public SessionData {
public:
	VGASessionData(Service *s,size_t id,capsel_t caps,Pt::portal_func func)
		: SessionData(s,id,caps,func), _sm(), _ec(receiver,next_cpu()), _sc(), _ds(), _cons() {
		_ec.set_tls<capsel_t>(Ec::TLS_PARAM,caps);
	}
	virtual ~VGASessionData() {
		delete _ds;
		delete _cons;
		delete _sc;
	}

	virtual void invalidate() {
		if(_cons)
			_cons->stop();
	}

	Consumer<Screen::Packet> *cons() {
		return _cons;
	}

protected:
	virtual void accept_ds(DataSpace *ds) {
		ScopedLock<UserSm> guard(&_sm);
		if(_ds != 0)
			throw Exception(E_EXISTS);
		_ds = ds;
		_cons = new Consumer<Screen::Packet>(ds,false);
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

	UserSm _sm;
	GlobalEc _ec;
	Sc *_sc;
	DataSpace *_ds;
	Consumer<Screen::Packet> *_cons;
	static CPU::iterator _cpu_it;
};

class VGAService : public Service {
public:
	VGAService(const char *name) : Service(name) {
	}

private:
	virtual SessionData *create_session(size_t id,capsel_t caps,Pt::portal_func func) {
		return new VGASessionData(this,id,caps,func);
	}
};

CPU::iterator VGASessionData::_cpu_it;
static HostVGA vga;
static VGAService *srv;

void VGASessionData::receiver(void *) {
	capsel_t caps = Ec::current()->get_tls<word_t>(Ec::TLS_PARAM);
	ScopedLock<RCULock> guard(&RCU::lock());
	VGASessionData *sess = srv->get_session<VGASessionData>(caps);
	Consumer<Screen::Packet> *cons = sess->cons();
	for(Screen::Packet *pk; (pk = cons->get()) != 0; cons->next()) {
		switch(pk->cmd) {
			case Screen::PAINT:
				vga.paint(*pk);
				break;
			case Screen::SCROLL:
				vga.scroll();
				break;
			case Screen::SETVIEW:
				vga.set_page(pk->view);
				break;
		}
	}
}

int main() {
	srv = new VGAService("screen");
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it)
		srv->provide_on(it->id);
	srv->reg();

	Sm sm(0);
	sm.down();
	return 0;
}
