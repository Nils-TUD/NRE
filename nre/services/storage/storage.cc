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

#include <kobj/Sm.h>
#include <ipc/Producer.h>
#include <services/PCIConfig.h>
#include <services/ACPI.h>
#include <util/PCI.h>
#include <Logging.h>
#include <cstring>

#include "ControllerMng.h"

using namespace nre;

class StorageService;

// TODO why does the VMT entry of HostAHCI::exists() point to StorageService::create_session()
// when we put the object here instead of a pointer??
static ControllerMng *mng;
static StorageService *srv;

class StorageServiceSession : public ServiceSession {
public:
	explicit StorageServiceSession(Service *s,size_t id,capsel_t caps,Pt::portal_func func)
		: ServiceSession(s,id,caps,func), _ctrlds(), _prod(), _datads(), _ctrl(), _drive() {
	}
	virtual ~StorageServiceSession() {
		delete _ctrlds;
		delete _prod;
		delete _datads;
	}

	bool initialized() const {
		return _ctrlds != 0;
	}
	size_t ctrl() const {
		return _ctrl;
	}
	size_t drive() const {
		return _drive;
	}
	const DataSpace &data() const {
		return *_datads;
	}
	const Storage::Parameter &params() const {
		return _params;
	}
	Producer<Storage::Packet> *prod() {
		return _prod;
	}

	void init(DataSpace *ctrlds,DataSpace *data,size_t ctrl,size_t drive) {
		if(!mng->exists(ctrl) || !mng->get(ctrl)->exists(drive))
			throw Exception(E_ARGS_INVALID,64,"Controller/drive (%zu,%zu) does not exist",ctrl,drive);
		if(_ctrlds)
			throw Exception(E_EXISTS,"Already initialized");
		_ctrlds = ctrlds;
		_prod = new Producer<Storage::Packet>(_ctrlds,false,false);
		_datads = data;
		_ctrl = ctrl;
		_drive = drive;
		mng->get(_ctrl)->get_params(_drive,&_params);
	}

private:
	DataSpace *_ctrlds;
	Producer<Storage::Packet> *_prod;
	DataSpace *_datads;
	size_t _ctrl;
	size_t _drive;
	Storage::Parameter _params;
};

class StorageService : public Service {
public:
	explicit StorageService(const char *name)
		: Service(name,CPUSet(CPUSet::ALL),portal) {
		// we want to accept two dataspaces
		for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it) {
			LocalThread *ec = get_thread(it->log_id());
			UtcbFrameRef uf(ec->utcb());
			uf.accept_delegates(1);
		}
	}

private:
	virtual ServiceSession *create_session(size_t id,capsel_t caps,Pt::portal_func func) {
		return new StorageServiceSession(this,id,caps,func);
	}

	PORTAL static void portal(capsel_t pid);
};

void StorageService::portal(capsel_t pid) {
	ScopedLock<RCULock> guard(&RCU::lock());
	StorageServiceSession *sess = srv->get_session<StorageServiceSession>(pid);
	UtcbFrameRef uf;
	try {
		Storage::Command cmd;
		uf >> cmd;
		switch(cmd) {
			case Storage::INIT: {
				capsel_t ctrlsel = uf.get_delegated(0).offset();
				capsel_t datasel = uf.get_delegated(0).offset();
				size_t ctrl,drive;
				uf >> ctrl >> drive;
				uf.finish_input();
				sess->init(new DataSpace(ctrlsel),new DataSpace(datasel),ctrl,drive);
				uf.accept_delegates();
				uf << E_SUCCESS;
			}
			break;

			case Storage::FLUSH: {
				Storage::tag_type tag;
				uf >> tag;
				uf.finish_input();
				LOG(Logging::STORAGE_DETAIL,Serial::get().writef("[%u,%#x] FLUSH\n",sess->id(),tag));
				mng->get(sess->ctrl())->flush(sess->drive(),sess->prod(),tag);
				uf << E_SUCCESS;
			}
			break;

			case Storage::GET_PARAMS: {
				uf.finish_input();
				LOG(Logging::STORAGE_DETAIL,Serial::get().writef("[%u] GET_PARAMS\n",sess->id()));
				uf << E_SUCCESS << sess->params();
			}
			break;

			case Storage::READ:
			case Storage::WRITE: {
				Storage::tag_type tag;
				Storage::sector_type sector,count;
				size_t offset;
				uf >> tag >> sector >> count >> offset;
				uf.finish_input();

				if(!sess->initialized())
					throw Exception(E_ARGS_INVALID,"Not initialized");

				LOG(Logging::STORAGE_DETAIL,Serial::get().writef("[%u,%#x] %s (%Lu..%Lu) @ %zu\n",sess->id(),
						tag,cmd == Storage::READ ? "READ" : "WRITE",sector,sector + count - 1,offset));

				if(cmd == Storage::READ) {
					if(!(sess->data().perm() & DataSpaceDesc::R))
						throw Exception(E_ARGS_INVALID,"Need to read, but no read permission");
					mng->get(sess->ctrl())->read(sess->drive(),sess->prod(),tag,
							sess->data(),sector,offset,count * sess->params().sector_size);
				}
				else {
					if(!(sess->data().perm() & DataSpaceDesc::W))
						throw Exception(E_ARGS_INVALID,"Need to write, but no write permission");
					mng->get(sess->ctrl())->write(sess->drive(),sess->prod(),tag,
							sess->data(),sector,offset,count * sess->params().sector_size);
				}
				uf << E_SUCCESS;
			}
			break;
		}
	}
	catch(const Exception &e) {
		Syscalls::revoke(uf.delegation_window(),true);
		uf.clear();
		uf << e;
	}
}

int main() {
	mng = new ControllerMng();
	srv = new StorageService("storage");
	srv->reg();
	Sm sm(0);
	sm.down();
	return 0;
}
