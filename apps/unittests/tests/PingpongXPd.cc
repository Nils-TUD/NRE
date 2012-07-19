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

#include <ipc/Service.h>
#include <ipc/Connection.h>
#include <ipc/Session.h>
#include <subsystem/ChildManager.h>
#include <kobj/Pt.h>
#include <utcb/UtcbFrame.h>
#include <util/Util.h>
#include <CPU.h>

#include "PingpongXPd.h"

using namespace nre;
using namespace nre::test;

class PingpongService;
static void test_pingpong();

const TestCase pingpongxpd = {
	"Cross-Pd PingPong",test_pingpong
};

static const uint tries = 10000;
static uint64_t results[tries];
static PingpongService *srv;

class PingpongSessionData : public SessionData {
public:
	PingpongSessionData(Service *s,size_t id,capsel_t caps,Pt::portal_func func)
		: SessionData(s,id,caps,func) {
	}

	virtual void invalidate();
};

class PingpongService : public Service {
public:
	PingpongService() : Service("pingpong",portal), _sm(0) {
		provide_on(CPU::current().log_id());
	}

	void wait() {
		_sm.down();
	}
	void notify() {
		_sm.up();
	}

private:
	virtual SessionData *create_session(size_t id,capsel_t caps,Pt::portal_func func) {
		return new PingpongSessionData(this,id,caps,func);
	}

	PORTAL static void portal(capsel_t pid);

	Sm _sm;
};

void PingpongSessionData::invalidate() {
	srv->notify();
}

void PingpongService::portal(capsel_t) {
	UtcbFrameRef uf;
	try {
		uint a,b,c;
		uf >> a >> b >> c;
		uf.clear();
		uf << (a + b) << (a + b + c);
	}
	catch(const Exception&) {
		uf.clear();
	}
}

static int pingpong_server() {
	srv = new PingpongService();
	srv->reg();
	srv->wait();
	delete srv;
	return 0;
}

static void pingpong_client() {
	Connection con("pingpong");
	Session sess(con);
	Pt pt(sess.caps() + CPU::current().log_id());
	uint64_t tic,tac,min = ~0ull,max = 0,ipc_duration,rdtsc;
	uint sum = 0;
	tic = Util::tsc();
	tac = Util::tsc();
	rdtsc = tac - tic;
	UtcbFrame uf;
	for(uint i = 0; i < tries; i++) {
		tic = Util::tsc();
		uf << 1 << 2 << 3;
		pt.call(uf);
		uint x = 0,y = 0;
		uf >> x >> y;
		uf.clear();
		sum += x + y;
		tac = Util::tsc();
		ipc_duration = tac - tic - rdtsc;
		min = Math::min(min,ipc_duration);
		max = Math::max(max,ipc_duration);
		results[i] = ipc_duration;
	}
	uint64_t avg = 0;
	for(uint i = 0; i < tries; i++)
		avg += results[i];

	avg = avg / tries;
	WVPERF(avg,"cycles");
	WVPASSEQ(sum,(1 + 2) * tries + (1 + 2 + 3) * tries);
	WVPRINTF("sum: %u",sum);
	WVPRINTF("avg: %Lu",avg);
	WVPRINTF("min: %Lu",min);
	WVPRINTF("max: %Lu",max);
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

static void test_pingpong() {
	ChildManager *mng = new ChildManager();
	Hip::mem_iterator self = get_self();
	// map the memory of the module
	DataSpace ds(self->size,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::R,self->addr);
	mng->load(ds.virt(),self->size,"pingpongservice provides=pingpong",reinterpret_cast<uintptr_t>(pingpong_server));
	mng->load(ds.virt(),self->size,"pingpongclient",reinterpret_cast<uintptr_t>(pingpong_client));
	mng->wait_for_all();
	delete mng;
}
