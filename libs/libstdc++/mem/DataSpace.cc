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
#include <Syscalls.h>
#include <CPU.h>

namespace nul {

void DataSpace::map() {
	UtcbFrame uf;
	CapHolder umcap,mcap;
	try {
		// if the sm-sel hasn't been passed in by IPC, create a new one
		if(_sel == ObjCap::INVALID)
			Syscalls::create_sm(mcap.get(),0,Pd::current()->sel());
		// prepare for receiving the unmap-cap
		uf.set_receive_crd(Crd(umcap.get(),0,DESC_CAP_ALL));
		if(_own)
			uf.delegate(_sel);
		else
			uf.translate(_sel);
		uf << 0 << _virt << _phys << _size << _perm << _type;
		CPU::current().map_pt->call(uf);
		// TODO error-handling
		uf >> _virt >> _size >> _perm >> _type;
		_unmapsel = umcap.release();
		if(_sel == ObjCap::INVALID)
			_sel = mcap.get();
		mcap.release();
	}
	catch(...) {
		// make sure that we don't leak the Sm
		if(_sel == ObjCap::INVALID)
			Syscalls::revoke(Crd(mcap.get(),0,DESC_CAP_ALL),true);
		throw;
	}
}

void DataSpace::unmap() {
	UtcbFrame uf;
	uf.translate(_unmapsel);
	uf << 1 << _virt << _phys << _size << _perm << _type;
	CPU::current().unmap_pt->call(uf);
	// TODO error-handling
	_unmapsel = ObjCap::INVALID;
	_sel = ObjCap::INVALID;
}

}
