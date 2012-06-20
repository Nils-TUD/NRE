/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <kobj/Pt.h>
#include <subsystem/ChildManager.h>
#include <service/Service.h>
#include <service/Consumer.h>
#include <service/Producer.h>
#include <service/Connection.h>
#include <service/Session.h>
#include <Hip.h>
#include <CPU.h>

#include "SharedMemory.h"

using namespace nul;
using namespace nul::test;

enum {
	DS_SIZE		= ExecEnv::PAGE_SIZE * 4,
	ITEM_COUNT	= 10000000,
	WORD_COUNT	= 1,
};

struct Item {
	word_t words[WORD_COUNT];
};
class ShmService;

static Hip::mem_iterator get_self();
static void test_shm();

const TestCase sharedmem = {
	"Shared memory",test_shm
};
static ShmService *srv;

class ShmSessionData : public SessionData {
public:
	ShmSessionData(Service *s,size_t id,capsel_t caps,Pt::portal_func func)
		: SessionData(s,id,caps,func), _ec(receiver,CPU::current().id), _sc(), _cons(), _ds() {
		_ec.set_tls<capsel_t>(Ec::TLS_PARAM,caps);
	}
	virtual ~ShmSessionData() {
		delete _ds;
		delete _cons;
		delete _sc;
	}

	virtual void invalidate() {
		if(_cons)
			_cons->stop();
	}

	Consumer<Item> *cons() {
		return _cons;
	}

protected:
	virtual void accept_ds(DataSpace *ds) {
		_ds = ds;
		_cons = new Consumer<Item>(ds);
		_sc = new Sc(&_ec,Qpd());
		_sc->start();
	}

private:
	static void receiver(void *);

	GlobalEc _ec;
	Sc *_sc;
	Consumer<Item> *_cons;
	DataSpace *_ds;
};

class ShmService : public Service {
public:
	ShmService() : Service("shm") {
	}

private:
	virtual SessionData *create_session(size_t id,capsel_t caps,Pt::portal_func func) {
		return new ShmSessionData(this,id,caps,func);
	}
};

void ShmSessionData::receiver(void*) {
	capsel_t caps = Ec::current()->get_tls<word_t>(Ec::TLS_PARAM);
	size_t count = 0;
	try {
		while(1) {
			ScopedLock<RCULock> guard(&RCU::lock());
			ShmSessionData *sess = srv->get_session<ShmSessionData>(caps);
			Consumer<Item> *cons = sess->cons();
			Item *w = cons->get();
			if(w == 0)
				break;
			count++;
			cons->next();
		}
	}
	catch(...) {
		// ignore
	}
	// TODO actually this might fail
	WVPASSEQ(count,static_cast<size_t>(ITEM_COUNT));
}

static void shm_service() {
	srv = new ShmService();
	srv->provide_on(CPU::current().id);
	srv->reg();
	srv->wait();
}

static void shm_client() {
	Connection con("shm");
	Session sess(con);
	DataSpace ds(DS_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW);
	Producer<Item> prod(&ds,true,true);
	ds.share(sess);
	uint64_t start = Util::tsc();
	Item i;
	for(word_t x = 0; x < ITEM_COUNT; ++x)
		prod.produce(i);
	uint64_t end = Util::tsc();
	uint64_t total = end - start;
	uint64_t avg = total / ITEM_COUNT;
	WVPRINTF("Transfered %u items",ITEM_COUNT);
	WVPERF(total,"Cycles");
	WVPERF(avg,"Cycles");
}

static Hip::mem_iterator get_self() {
	int i = 0;
	const Hip &hip = Hip::get();
	for(Hip::mem_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		// we're the third module (root and log come first)
		if(it->type == Hip_mem::MB_MODULE && i++ > 1)
			return it;
	}
	return hip.mem_end();
}

static void test_shm() {
	ChildManager *mng = new ChildManager();
	Hip::mem_iterator self = get_self();
	// map the memory of the module
	DataSpace *ds = new DataSpace(self->size,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::R,self->addr);
	mng->load(ds->virt(),self->size,"shm_service provides=shm",
			reinterpret_cast<uintptr_t>(shm_service));
	mng->load(ds->virt(),self->size,"shm_client",
			reinterpret_cast<uintptr_t>(shm_client));
	Sm sm(0);
	sm.down();
}
