/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <kobj/UserSm.h>
#include <kobj/Gsi.h>
#include <kobj/Ports.h>
#include <utcb/UtcbFrame.h>
#include <ScopedLock.h>

#include "Hypervisor.h"

using namespace nul;

uchar Hypervisor::_stack[ExecEnv::PAGE_SIZE] ALIGNED(ExecEnv::PAGE_SIZE);
Pt *Hypervisor::_mem_pt;
RegionManager Hypervisor::_io;
BitField<Hip::MAX_GSIS> Hypervisor::_gsis;
UserSm *Hypervisor::_io_sm;
UserSm *Hypervisor::_gsi_sm;

void Hypervisor::init() {
	LocalEc *ec = new LocalEc(CPU::current().id,ObjCap::INVALID,reinterpret_cast<uintptr_t>(_stack));
	_io_sm = new UserSm();
	_gsi_sm = new UserSm();
	_mem_pt = new Pt(ec,portal_mem);
	// make all I/O ports available
	_io.free(0,0xFFFF);
}

void Hypervisor::allocate_mem(uintptr_t phys,uintptr_t virt,size_t size) {
	UtcbFrame uf;
	uf.set_receive_crd(Crd(0,31,Crd::MEM_ALL));
	size_t pages = Math::blockcount<size_t>(size,ExecEnv::PAGE_SIZE);
	CapRange cr(phys >> ExecEnv::PAGE_SHIFT,pages,Crd::MEM_ALL,virt >> ExecEnv::PAGE_SHIFT);
	size_t count = pages;
	while(count > 0) {
		uf.clear();
		if(cr.hotspot()) {
			// if start and hotspot are different, we have to check whether it fits in the utcb
			uintptr_t diff = cr.hotspot() ^ cr.start();
			if(diff) {
				// the lowest bit that's different defines how many we can map with one Crd.
				// with bit 0, its 2^0 = 1 at once, with bit 1, 2^1 = 2 and so on.
				unsigned at_once = Math::bit_scan_forward(diff);
				if((1 << at_once) < count) {
					// be carefull that we might have to align it to 1 << at_once first. this takes
					// at most at_once typed items.
					size_t min = Math::min<uintptr_t>((uf.freewords() / 2) - at_once,count >> at_once);
					cr.count(min << at_once);
				}
			}
		}

		uf << cr;
		_mem_pt->call(uf);

		// adjust start and hotspot for the next round
		cr.start(cr.start() + cr.count());
		if(cr.hotspot())
			cr.hotspot(cr.hotspot() + cr.count());
		count -= cr.count();
		cr.count(count);
	}
}

void Hypervisor::portal_mem(capsel_t) {
	UtcbFrameRef uf;
	CapRange range;
	uf >> range;
	uf.clear();
	uf.delegate(range,UtcbFrame::FROM_HV);
}

void Hypervisor::portal_gsi(capsel_t) {
	UtcbFrameRef uf;
	try {
		uint gsi;
		Gsi::Op op;
		uf >> op >> gsi;
		uf.finish_input();
		// we can trust the values because it's send from ourself
		assert(gsi < Hip::MAX_GSIS);

		switch(op) {
			case Gsi::ALLOC:
				allocate_gsi(gsi);
				break;

			case Gsi::RELEASE:
				release_gsi(gsi);
				break;
		}

		if(op == Gsi::ALLOC)
			uf.delegate(Hip::MAX_CPUS + gsi,0,UtcbFrame::FROM_HV);
		uf << E_SUCCESS;
	}
	catch(const Exception& e) {
		uf.clear();
		uf << e.code();
	}
}

void Hypervisor::portal_io(capsel_t) {
	UtcbFrameRef uf;
	try {
		Ports::port_t base;
		uint count;
		Ports::Op op;
		uf >> op >> base >> count;
		uf.finish_input();

		switch(op) {
			case Ports::ALLOC:
				allocate_ports(base,count);
				break;

			case Ports::RELEASE:
				release_ports(base,count);
				// TODO revoke ports
				break;
		}

		if(op == Ports::ALLOC)
			uf.delegate(CapRange(base,count,Crd::IO_ALL),UtcbFrame::FROM_HV);
		uf << E_SUCCESS;
	}
	catch(const Exception& e) {
		uf.clear();
		uf << e.code();
	}
}
