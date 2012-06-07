/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <mem/DataSpace.h>
#include <cap/CapHolder.h>
#include <kobj/Pd.h>
#include <kobj/Pt.h>
#include <utcb/UtcbFrame.h>
#include <service/Session.h>
#include <service/Service.h>
#include <stream/Serial.h>
#include <Syscalls.h>
#include <CPU.h>

namespace nul {

void DataSpace::handle_response(UtcbFrameRef& uf) {
	ErrorCode res;
	uf >> res;
	if(res != E_SUCCESS)
		throw DataSpaceException(res);
}

void DataSpace::create() {
	assert(_sel == ObjCap::INVALID && _unmapsel == ObjCap::INVALID);
	UtcbFrame uf;

	// prepare for receiving map and unmap-cap
	CapHolder caps(2,2);
	uf.set_receive_crd(Crd(caps.get(),1,DESC_CAP_ALL));
	uf << CREATE << _virt << _phys << _size << _perm << _type;
	CPU::current().map_pt->call(uf);

	handle_response(uf);
	uf >> _virt >> _size >> _perm >> _type;
	_sel = caps.get();
	_unmapsel = caps.get() + 1;
	caps.release();
}

void DataSpace::share(Session &c) {
	UtcbFrame uf;
	// for the service protocol (identifies our session)
	uf.translate(c.pt(CPU::current().id).sel());
	uf << Service::SHARE_DATASPACE;
	// for the dataspace protocol
	uf << JOIN << 0 << 0 << _size << _perm << _type;
	uf.delegate(_sel);
	c.con().pt(CPU::current().id)->call(uf);
	handle_response(uf);
}

void DataSpace::join() {
	assert(_sel != ObjCap::INVALID && _unmapsel == ObjCap::INVALID);
	UtcbFrame uf;

	// prepare for receiving unmap-cap
	CapHolder umcap;
	uf.set_receive_crd(Crd(umcap.get(),0,DESC_CAP_ALL));
	uf.translate(_sel);
	uf << JOIN << _virt << _phys << _size << _perm << _type;
	CPU::current().map_pt->call(uf);

	handle_response(uf);
	uf >> _virt >> _size >> _perm >> _type;
	_unmapsel = umcap.release();
}

void DataSpace::unmap() {
	assert(_sel != ObjCap::INVALID && _unmapsel != ObjCap::INVALID);
	UtcbFrame uf;

	uf.translate(_unmapsel);
	uf << DESTROY << _virt << _phys << _size << _perm << _type;
	CPU::current().unmap_pt->call(uf);

	CapSpace::get().free(_unmapsel);
	CapSpace::get().free(_sel);
	_unmapsel = ObjCap::INVALID;
	_sel = ObjCap::INVALID;
}

UtcbFrameRef &operator >>(UtcbFrameRef &uf,DataSpace &ds) {
	DataSpace::RequestType type;
	uf >> type >> ds._virt >> ds._phys >> ds._size >> ds._perm >> ds._type;
	if(ds._type != DataSpace::ANONYMOUS && ds._type != DataSpace::LOCKED)
		throw DataSpaceException(E_ARGS_INVALID);
	if(ds._size == 0 || (ds._perm & DataSpace::RWX) == 0)
		throw DataSpaceException(E_ARGS_INVALID);
	// TODO check phys

	TypedItem ti;
	switch(type) {
		case DataSpace::CREATE:
			break;
		case DataSpace::JOIN:
			uf.get_typed(ti);
			ds._sel = ti.crd().cap();
			break;
		case DataSpace::DESTROY:
			uf.get_typed(ti);
			ds._unmapsel = ti.crd().cap();
			break;
	}
	return uf;
}

}
