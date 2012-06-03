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
#include <service/Client.h>
#include <stream/Serial.h>
#include <Syscalls.h>
#include <CPU.h>

namespace nul {

void DataSpace::create() {
	Serial::get().writef("### Before Create: ");
	this->write(Serial::get());
	Serial::get().writef("\n");

	assert(_sel == ObjCap::INVALID && _unmapsel == ObjCap::INVALID);
	UtcbFrame uf;
	CapHolder caps(2,2);
	// prepare for receiving map and unmap-cap
	uf.set_receive_crd(Crd(caps.get(),1,DESC_CAP_ALL));
	uf << CREATE << _virt << _phys << _size << _perm << _type;
	CPU::current().map_pt->call(uf);
	// TODO error-handling
	uf >> _virt >> _size >> _perm >> _type;
	_sel = caps.get();
	_unmapsel = caps.get() + 1;
	caps.release();

	Serial::get().writef("### After Create: ");
	this->write(Serial::get());
	Serial::get().writef("\n");
}

void DataSpace::share(Client &c) {
	UtcbFrame uf;
	uf << JOIN << 0 << 0 << _size << _perm << _type;
	uf.delegate(_sel);
	c.shpt()->call(uf);
}

void DataSpace::join() {
	Serial::get().writef("### Before Join: ");
	this->write(Serial::get());
	Serial::get().writef("\n");

	assert(_sel != ObjCap::INVALID && _unmapsel == ObjCap::INVALID);
	UtcbFrame uf;
	CapHolder umcap;
	// prepare for receiving unmap-cap
	uf.set_receive_crd(Crd(umcap.get(),0,DESC_CAP_ALL));
	uf.translate(_sel);
	uf << JOIN << _virt << _phys << _size << _perm << _type;
	CPU::current().map_pt->call(uf);
	// TODO error-handling
	uf >> _virt >> _size >> _perm >> _type;
	_unmapsel = umcap.release();

	Serial::get().writef("### After Join: ");
	this->write(Serial::get());
	Serial::get().writef("\n");
}

void DataSpace::unmap() {
	assert(_sel != ObjCap::INVALID && _unmapsel != ObjCap::INVALID);
	UtcbFrame uf;
	uf.translate(_unmapsel);
	uf << DESTROY << _virt << _phys << _size << _perm << _type;
	CPU::current().unmap_pt->call(uf);
	// TODO error-handling
	CapSpace::get().free(_unmapsel);
	CapSpace::get().free(_sel);
	_unmapsel = ObjCap::INVALID;
	_sel = ObjCap::INVALID;
}

UtcbFrameRef &operator >>(UtcbFrameRef &uf,DataSpace &ds) {
	DataSpace::RequestType type;
	// TODO check for errors
	uf >> type >> ds._virt >> ds._phys >> ds._size >> ds._perm >> ds._type;
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
