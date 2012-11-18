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
    explicit StorageServiceSession(Service *s, size_t id, capsel_t cap, capsel_t caps,
                                   Pt::portal_func func)
        : ServiceSession(s, id, cap, caps, func), _ctrlds(), _prod(), _datads(), _drive() {
    }
    virtual ~StorageServiceSession() {
        delete _ctrlds;
        delete _prod;
        delete _datads;
    }

    bool initialized() const {
        return _ctrlds != 0;
    }
    size_t drive() const {
        return _drive;
    }
    size_t ctrl() const {
        return _drive / Storage::MAX_DRIVES;
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

    void init(DataSpace *ctrlds, DataSpace *data, size_t drive) {
        size_t ctrl = drive / Storage::MAX_DRIVES;
        if(!mng->exists(ctrl) || !mng->get(ctrl)->exists(drive)) {
            VTHROW(Exception, E_ARGS_INVALID,
                   "Controller/drive (" << ctrl << "," << drive << ") does not exist");
        }
        if(_ctrlds)
            throw Exception(E_EXISTS, "Already initialized");
        _ctrlds = ctrlds;
        _prod = new Producer<Storage::Packet>(_ctrlds, false);
        _datads = data;
        _drive = drive;
        mng->get(ctrl)->get_params(_drive, &_params);
    }

private:
    DataSpace *_ctrlds;
    Producer<Storage::Packet> *_prod;
    DataSpace *_datads;
    size_t _drive;
    Storage::Parameter _params;
};

class StorageService : public Service {
public:
    explicit StorageService(const char *name)
        : Service(name, CPUSet(CPUSet::ALL), portal) {
        // we want to accept two dataspaces
        for(auto it = CPU::begin(); it != CPU::end(); ++it) {
            LocalThread *ec = get_thread(it->log_id());
            UtcbFrameRef uf(ec->utcb());
            uf.accept_delegates(1);
        }
    }

private:
    virtual ServiceSession *create_session(size_t id, capsel_t cap, capsel_t caps,
                                           Pt::portal_func func) {
        return new StorageServiceSession(this, id, cap, caps, func);
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
                size_t drive;
                uf >> drive;
                uf.finish_input();
                sess->init(new DataSpace(ctrlsel), new DataSpace(datasel), drive);
                uf.accept_delegates();
                uf << E_SUCCESS << sess->params();
            }
            break;

            case Storage::FLUSH: {
                Storage::tag_type tag;
                uf >> tag;
                uf.finish_input();
                LOG(STORAGE_DETAIL, "[" << sess->id() << "," << fmt(tag, "#x") << "] FLUSH\n");
                mng->get(sess->ctrl())->flush(sess->drive(), sess->prod(), tag);
                uf << E_SUCCESS;
            }
            break;

            case Storage::READ:
            case Storage::WRITE: {
                Storage::tag_type tag;
                Storage::sector_type sector;
                DMADescList<Storage::MAX_DMA_DESCS> dma;
                uf >> tag >> sector >> dma;
                uf.finish_input();

                if(!sess->initialized())
                    throw Exception(E_ARGS_INVALID, "Not initialized");

                LOG(STORAGE_DETAIL, "[" << sess->id() << "," << fmt(tag, "#x") << "] "
                                        << (cmd == Storage::READ ? "READ" : "WRITE") << " @ " << sector
                                        << " with " << dma << "\n");

                // check offset and size
                size_t size = dma.bytecount();
                size_t count = size / sess->params().sector_size;
                if(size == 0 || (size & (sess->params().sector_size - 1)))
                    VTHROW(Exception, E_ARGS_INVALID, "Invalid size (" << size << ")");
                if(sector >= sess->params().sectors) {
                    VTHROW(Exception, E_ARGS_INVALID,
                           "Sector " << sector << " is invalid"
                                     << " (available: 0.." << sess->params().sectors - 1 << ")");
                }
                if(sector + count > sess->params().sectors) {
                    VTHROW(Exception, E_ARGS_INVALID,
                           "Sector " << (sector + count - 1) << " is invalid"
                                     << " (available: 0.." << sess->params().sectors - 1 << ")");
                }

                if(cmd == Storage::READ) {
                    if(!(sess->data().flags() & DataSpaceDesc::R))
                        throw Exception(E_ARGS_INVALID, "Need to read, but no read permission");
                    mng->get(sess->ctrl())->read(sess->drive(), sess->prod(), tag,
                                                 sess->data(), sector, dma);
                }
                else {
                    if(!(sess->data().flags() & DataSpaceDesc::W))
                        throw Exception(E_ARGS_INVALID, "Need to write, but no write permission");
                    mng->get(sess->ctrl())->write(sess->drive(), sess->prod(), tag, sess->data(), sector,
                                                  dma);
                }
                uf << E_SUCCESS;
            }
            break;
        }
    }
    catch(const Exception &e) {
        Syscalls::revoke(uf.delegation_window(), true);
        uf.clear();
        uf << e;
    }
}

int main(int argc, char *argv[]) {
    bool idedma = true;
    for(int i = 1; i < argc; ++i) {
        if(strcmp(argv[i], "noidedma") == 0) {
            LOG(STORAGE, "Disabling DMA for IDE devices\n");
            idedma = false;
        }
    }

    mng = new ControllerMng(idedma);
    srv = new StorageService("storage");
    srv->start();
    return 0;
}
