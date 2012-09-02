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

#include <kobj/Pt.h>
#include <kobj/Sc.h>
#include <subsystem/ChildManager.h>
#include <ipc/Service.h>
#include <ipc/Consumer.h>
#include <ipc/Producer.h>
#include <ipc/Connection.h>
#include <ipc/ClientSession.h>
#include <stream/IStringStream.h>
#include <Hip.h>
#include <CPU.h>

#include "SharedMemory.h"

using namespace nre;
using namespace nre::test;

enum {
	DS_SIZE		= ExecEnv::PAGE_SIZE * 4,
	ITEM_COUNT	= 1000000,
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

class ShmSession : public ServiceSession {
public:
	explicit ShmSession(Service *s,size_t id,capsel_t caps,Pt::portal_func func)
		: ServiceSession(s,id,caps,func), _ec(receiver,CPU::current().log_id()), _sc(), _cons(), _ds() {
		_ec.set_tls<capsel_t>(Thread::TLS_PARAM,caps);
	}
	virtual ~ShmSession() {
		delete _ds;
		delete _cons;
		delete _sc;
	}

	virtual void invalidate();

	Consumer<Item> *cons() {
		return _cons;
	}

	void set_ds(DataSpace *ds) {
		_ds = ds;
		_cons = new Consumer<Item>(ds);
		_sc = new Sc(&_ec,Qpd());
		_sc->start(String("shm-receiver"));
	}

private:
	static void receiver(void *);

	GlobalThread _ec;
	Sc *_sc;
	Consumer<Item> *_cons;
	DataSpace *_ds;
};

class ShmService : public Service {
public:
	explicit ShmService() : Service("shm",CPUSet(CPUSet::ALL),portal), _sm(0) {
		UtcbFrameRef uf(get_thread(CPU::current().log_id())->utcb());
		uf.accept_delegates(0);
	}

	void wait() {
		_sm.down();
	}
	void notify() {
		_sm.up();
	}

private:
	PORTAL static void portal(capsel_t pid);

	virtual ServiceSession *create_session(size_t id,capsel_t caps,Pt::portal_func func) {
		return new ShmSession(this,id,caps,func);
	}

	Sm _sm;
};

void ShmService::portal(capsel_t pid) {
	UtcbFrameRef uf;
	try {
		ShmSession *sess = srv->get_session<ShmSession>(pid);
		capsel_t sel = uf.get_delegated(0).offset();
		uf.finish_input();
		sess->set_ds(new DataSpace(sel));
		uf << E_SUCCESS;
	}
	catch(const Exception &e) {
		uf.clear();
		uf << e;
	}
}

void ShmSession::invalidate() {
	srv->notify();
	if(_cons)
		_cons->stop();
}

void ShmSession::receiver(void*) {
	capsel_t caps = Thread::current()->get_tls<word_t>(Thread::TLS_PARAM);
	ScopedLock<RCULock> guard(&RCU::lock());
	ShmSession *sess = srv->get_session<ShmSession>(caps);
	Consumer<Item> *cons = sess->cons();
	size_t count = 0;
	while(1) {
		Item *w = cons->get();
		if(w == 0)
			break;
		count++;
		cons->next();
	}
	WVPASSEQ(count,static_cast<size_t>(ITEM_COUNT));
	srv->notify();
}

static int shm_service(int argc,char *argv[]) {
	for(int i = 0; i < argc; ++i)
		Serial::get() << "arg " << i << ": " << argv[i] << "\n";

	srv = new ShmService();
	srv->reg();
	srv->wait();
	srv->unreg();
	delete srv;
	return 0;
}

static int shm_client(int,char *argv[]) {
	size_t ds_size = IStringStream::read_from<size_t>(argv[1]);
	Connection con("shm");
	ClientSession sess(con);
	DataSpace ds(ds_size,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW);
	Producer<Item> prod(&ds,true,true);
	{
		UtcbFrame uf;
		uf.delegate(ds.sel());
		Pt pt(sess.caps() + CPU::current().log_id());
		pt.call(uf);
		uf.check_reply();
	}
	Item i;
	uint64_t start = Util::tsc();
	for(uint x = 0; x < ITEM_COUNT; ++x)
		prod.produce(i);
	uint64_t end = Util::tsc();
	uint64_t total = end - start;
	uint64_t avg = total / ITEM_COUNT;
	WVPRINTF("Transfered %u items",ITEM_COUNT);
	WVPERF(total,"Cycles");
	WVPERF(avg,"Cycles");
	return 0;
}

static Hip::mem_iterator get_self() {
	int i = 0;
	const Hip &hip = Hip::get();
	for(Hip::mem_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		// we're the second module
		if(it->type == HipMem::MB_MODULE && i++ > 0)
			return it;
	}
	return hip.mem_end();
}

static void test_shm() {
	char cmdline[64];
	size_t ds_sizes[] = {ExecEnv::PAGE_SIZE,ExecEnv::PAGE_SIZE * 2,ExecEnv::PAGE_SIZE * 4};
	for(size_t i = 0; i < ARRAY_SIZE(ds_sizes); ++i) {
		ChildManager *mng = new ChildManager();
		Hip::mem_iterator self = get_self();
		// map the memory of the module
		DataSpace *ds = new DataSpace(self->size,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::R,self->addr);
		OStringStream os(cmdline,sizeof(cmdline));
		os << "shm_client " << ds_sizes[i];
		mng->load(ds->virt(),self->size,"shm_service provides=shm",reinterpret_cast<uintptr_t>(shm_service));
		mng->load(ds->virt(),self->size,cmdline,reinterpret_cast<uintptr_t>(shm_client));
		mng->wait_for_all();
		delete ds;
		delete mng;
	}
}
