/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <mem/DataSpace.h>
#include <kobj/Pd.h>
#include <kobj/Pt.h>
#include <utcb/UtcbFrame.h>
#include <service/Session.h>
#include <service/Service.h>
#include <stream/Serial.h>
#include <util/ScopedCapSels.h>
#include <Syscalls.h>
#include <CPU.h>

namespace nul {

void DataSpace::handle_response(UtcbFrameRef& uf) {
	ErrorCode res;
	uf >> res;
	if(res != E_SUCCESS)
		throw DataSpaceException(res);
}

void DataSpace::create(DataSpaceDesc &desc,capsel_t *sel,capsel_t *unmapsel) {
	UtcbFrame uf;
	// prepare for receiving map and unmap-cap
	ScopedCapSels caps(2,2);
	uf.set_receive_crd(Crd(caps.get(),1,Crd::OBJ_ALL));
	uf << CREATE << desc;
	CPU::current().map_pt->call(uf);

	handle_response(uf);
	uf >> desc;
	if(sel)
		*sel = caps.get();
	if(unmapsel)
		*unmapsel = caps.get() + 1;
	caps.release();
}

void DataSpace::create() {
	assert(_sel == ObjCap::INVALID && _unmapsel == ObjCap::INVALID);
	create(_desc,&_sel,&_unmapsel);
}

void DataSpace::share(Session &s) {
	UtcbFrame uf;
	// for the service protocol (identifies our session)
	uf.translate(s.caps());
	uf << Service::SHARE_DATASPACE;
	// for the dataspace protocol
	uf << SHARE << _desc;
	uf.delegate(_sel);
	s.con().pt(CPU::current().id)->call(uf);
	handle_response(uf);
}

void DataSpace::join() {
	assert(_sel != ObjCap::INVALID && _unmapsel == ObjCap::INVALID);
	UtcbFrame uf;

	// prepare for receiving unmap-cap
	ScopedCapSels umcap;
	uf.set_receive_crd(Crd(umcap.get(),0,Crd::OBJ_ALL));
	uf.translate(_sel);
	uf << JOIN << _desc;
	CPU::current().map_pt->call(uf);

	handle_response(uf);
	uf >> _desc;
	_unmapsel = umcap.release();
}

void DataSpace::destroy() {
	assert(_sel != ObjCap::INVALID && _unmapsel != ObjCap::INVALID);
	UtcbFrame uf;

	uf.translate(_unmapsel);
	uf << DESTROY << _desc;
	CPU::current().unmap_pt->call(uf);

	CapSpace::get().free(_unmapsel);
	CapSpace::get().free(_sel);
}

}
