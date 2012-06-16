/*
 * TODO comment me
 *
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NUL.
 *
 * NUL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NUL is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#include <subsystem/ChildManager.h>
#include <stream/Serial.h>
#include <kobj/Gsi.h>
#include <kobj/Ports.h>
#include <cap/Caps.h>
#include <arch/Elf.h>
#include <Math.h>

namespace nul {

size_t ChildManager::_cpu_count = Hip::get().cpu_online_count();

ChildManager::ChildManager() : _child(), _childs(),
		_portal_caps(CapSpace::get().allocate(MAX_CHILDS * per_child_caps(),per_child_caps())),
		_dsm(), _registry(), _sm(), _regsm(0), _ecs(), _regecs() {
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it) {
		LocalEc **ecs[] = {_ecs,_regecs};
		for(size_t i = 0; i < sizeof(ecs) / sizeof(ecs[0]); ++i) {
			ecs[i][it->id] = new LocalEc(it->id);
			ecs[i][it->id]->set_tls(Ec::TLS_PARAM,this);
		}

		UtcbFrameRef defuf(_ecs[it->id]->utcb());
		defuf.accept_translates();
		defuf.accept_delegates(0);

		UtcbFrameRef reguf(_regecs[it->id]->utcb());
		reguf.accept_delegates(Math::next_pow2_shift<size_t>(Hip::MAX_CPUS));
	}
}

ChildManager::~ChildManager() {
	for(size_t i = 0; i < MAX_CHILDS; ++i)
		delete _childs[i];
	for(size_t i = 0; i < Hip::MAX_CPUS; ++i) {
		delete _ecs[i];
		delete _regecs[i];
	}
}

void ChildManager::load(uintptr_t addr,size_t size,const char *cmdline) {
	ElfEh *elf = reinterpret_cast<ElfEh*>(addr);

	// check ELF
	if(size < sizeof(ElfEh) || sizeof(ElfPh) > elf->e_phentsize ||
			size < elf->e_phoff + elf->e_phentsize * elf->e_phnum)
		throw ElfException(E_ELF_INVALID);
	if(!(elf->e_ident[0] == 0x7f && elf->e_ident[1] == 'E' &&
			elf->e_ident[2] == 'L' && elf->e_ident[3] == 'F'))
		throw ElfException(E_ELF_SIG);

	// create child
	Child *c = new Child(cmdline);
	try {
		// we have to create the portals first to be able to delegate them to the new Pd
		capsel_t pts = _portal_caps + _child * per_child_caps();
		c->_ptcount = _cpu_count * Portals::COUNT;
		c->_pts = new Pt*[c->_ptcount];
		for(cpu_t cpu = 0; cpu < _cpu_count; ++cpu) {
			size_t idx = cpu * Portals::COUNT;
			size_t off = cpu * Hip::get().service_caps();
			c->_pts[idx + 0] = new Pt(_ecs[cpu],pts + off + CapSpace::EV_PAGEFAULT,Portals::pf,
					Mtd(Mtd::GPR_BSD | Mtd::QUAL | Mtd::RIP_LEN));
			c->_pts[idx + 1] = new Pt(_ecs[cpu],pts + off + CapSpace::EV_STARTUP,Portals::startup,
					Mtd(Mtd::RSP));
			c->_pts[idx + 2] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_INIT,Portals::init_caps,0);
			c->_pts[idx + 3] = new Pt(_regecs[cpu],pts + off + CapSpace::SRV_REG,Portals::reg,0);
			c->_pts[idx + 4] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_UNREG,Portals::unreg,0);
			c->_pts[idx + 5] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_GET,Portals::get_service,0);
			c->_pts[idx + 6] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_IO,Portals::io,0);
			c->_pts[idx + 7] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_GSI,Portals::gsi,0);
			c->_pts[idx + 8] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_MAP,Portals::map,0);
			c->_pts[idx + 9] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_UNMAP,Portals::unmap,0);
		}
		// now create Pd and pass portals
		c->_pd = new Pd(Crd(pts,Math::next_pow2_shift(per_child_caps()),Crd::OBJ_ALL));
		// TODO wrong place
		c->_utcb = 0x7FFFF000;
		c->_ec = new GlobalEc(reinterpret_cast<GlobalEc::startup_func>(elf->e_entry),
				0,0,c->_pd,c->_utcb);
		c->_entry = elf->e_entry;

		// check load segments and add them to regions
		for(size_t i = 0; i < elf->e_phnum; i++) {
			ElfPh *ph = reinterpret_cast<ElfPh*>(addr + elf->e_phoff + i * elf->e_phentsize);
			if(reinterpret_cast<uintptr_t>(ph) + sizeof(ElfPh) > addr + size)
				throw ElfException(E_ELF_INVALID);
			if(ph->p_type != 1)
				continue;
			if(size < ph->p_offset + ph->p_filesz)
				throw ElfException(E_ELF_INVALID);

			uint perms = 0;
			if(ph->p_flags & PF_R)
				perms |= ChildMemory::R;
			if(ph->p_flags & PF_W)
				perms |= ChildMemory::W;
			if(ph->p_flags & PF_X)
				perms |= ChildMemory::X;

			DataSpace ds(Math::round_up<size_t>(ph->p_memsz,ExecEnv::PAGE_SIZE),DataSpace::ANONYMOUS,ChildMemory::RWX);
			// TODO leak, if reglist().add throws
			_dsm.create(ds);
			// TODO actually it would be better to do that later
			memcpy(reinterpret_cast<void*>(ds.virt()),
					reinterpret_cast<void*>(addr + ph->p_offset),ph->p_filesz);
			memset(reinterpret_cast<void*>(ds.virt() + ph->p_filesz),0,ph->p_memsz - ph->p_filesz);
			c->reglist().add(ds,ph->p_vaddr,perms);
		}

		// he needs a stack
		c->_stack = c->reglist().find_free(ExecEnv::STACK_SIZE);
		c->reglist().add(c->stack(),ExecEnv::STACK_SIZE,c->_ec->stack().virt(),ChildMemory::RW);
		// and a HIP
		{
			DataSpace ds(ExecEnv::PAGE_SIZE,DataSpace::ANONYMOUS,DataSpace::RW);
			_dsm.create(ds);
			c->_hip = c->reglist().find_free(ExecEnv::PAGE_SIZE);
			memcpy(reinterpret_cast<void*>(ds.virt()),&Hip::get(),ExecEnv::PAGE_SIZE);
			// TODO we need to adjust the hip
			c->reglist().add(ds,c->hip(),DataSpace::R);
		}

		Serial::get().writef("Starting client '%s'...\n",c->cmdline());
		c->reglist().write(Serial::get());
		Serial::get().writef("\n");

		// start child; we have to put the child into the list before that
		_childs[_child++] = c;
		c->_sc = new Sc(c->_ec,Qpd(),c->_pd);
		c->_sc->start();
	}
	catch(...) {
		delete c;
		_childs[--_child] = 0;
		throw;
	}

	// TODO well, we should make this more general
	const char *provides = strstr(cmdline,"provides=");
	if(provides != 0) {
		provides += sizeof("provides=") - 1;
		size_t i;
		const char *cur = provides;
		for(i = 0; *cur >= 'a' && *cur <= 'z'; ++i)
			cur++;

		// wait until the service has been registered
		String name(provides,i);
		Serial::get().writef("Waiting for '%s'...\n",name.str());
		while(_registry.find(name) == 0)
			_regsm.down();
		Serial::get().writef("Found '%s'!\n",name.str());
	}
}

void ChildManager::Portals::startup(capsel_t pid) {
	ChildManager *cm = Ec::current()->get_tls<ChildManager*>(Ec::TLS_PARAM);
	UtcbExcFrameRef uf;
	try {
		Child *c = cm->get_child(pid);
		if(c->_started) {
			uintptr_t src;
			size_t size;
			if(!c->reglist().find(uf->rsp,src,size))
				throw ChildMemoryException(E_NOT_FOUND);
			uf->rip = *reinterpret_cast<word_t*>(src + (uf->rsp & (ExecEnv::PAGE_SIZE - 1)) + sizeof(word_t));
			uf->mtd = Mtd::RIP_LEN;
			c->increase_refs();
		}
		else {
			uf->rip = *reinterpret_cast<word_t*>(uf->rsp + sizeof(word_t));
			uf->rsp = c->stack() + (uf->rsp & (ExecEnv::PAGE_SIZE - 1));
			// the bit indicates that its not the root-task
			// TODO not nice
	#ifdef __i386__
			uf->rax = (1 << 31) | c->_ec->cpu();
	#else
			uf->rdi = (1 << 31) | c->_ec->cpu();
	#endif
			uf->rcx = c->hip();
			uf->rdx = c->utcb();
			uf->mtd = Mtd::RIP_LEN | Mtd::RSP | Mtd::GPR_ACDB | Mtd::GPR_BSD;
			c->_started = true;
		}
	}
	catch(...) {
		// let the kernel kill the Ec
		uf->rip = ExecEnv::KERNEL_START;
		uf->mtd = Mtd::RIP_LEN;
	}
}

void ChildManager::Portals::init_caps(capsel_t pid) {
	ChildManager *cm = Ec::current()->get_tls<ChildManager*>(Ec::TLS_PARAM);
	UtcbFrameRef uf;
	try {
		Child *c = cm->get_child(pid);
		// we can't give the child the cap for e.g. the Pd when creating the Pd. therefore the child
		// grabs them afterwards with this portal
		uf.finish_input();
		uf.delegate(c->_pd->sel(),0);
		uf.delegate(c->_ec->sel(),1);
		uf.delegate(c->_sc->sel(),2);
		uf << E_SUCCESS;
	}
	catch(const Exception& e) {
		Syscalls::revoke(uf.get_receive_crd(),true);
		uf.clear();
		uf << e.code();
	}
}

void ChildManager::Portals::reg(capsel_t pid) {
	ChildManager *cm = Ec::current()->get_tls<ChildManager*>(Ec::TLS_PARAM);
	UtcbFrameRef uf;
	try {
		Child *c = cm->get_child(pid);
		String name;
		BitField<Hip::MAX_CPUS> available;
		capsel_t cap = uf.get_delegated(uf.get_receive_crd().order()).cap();
		uf >> name;
		uf >> available;
		uf.finish_input();

		cm->reg_service(c,cap,name,available);

		uf.accept_delegates();
		uf << E_SUCCESS;
	}
	catch(const Exception& e) {
		Syscalls::revoke(uf.get_receive_crd(),true);
		uf.clear();
		uf << e.code();
	}
}

void ChildManager::Portals::unreg(capsel_t pid) {
	ChildManager *cm = Ec::current()->get_tls<ChildManager*>(Ec::TLS_PARAM);
	UtcbFrameRef uf;
	// TODO doesn't work; we need to make sure that only the owner of the service can use that
	try {
		Child *c = cm->get_child(pid);
		String name;
		uf >> name;
		uf.finish_input();

		cm->unreg_service(c,name);

		uf << E_SUCCESS;
	}
	catch(const Exception& e) {
		Syscalls::revoke(uf.get_receive_crd(),true);
		uf.clear();
		uf << e.code();
	}
}

// TODO code-duplication; we already have that in the Session class
capsel_t ChildManager::get_parent_service(const char *name,BitField<Hip::MAX_CPUS> &available) {
	UtcbFrame uf;
	CapHolder caps(Hip::MAX_CPUS,Hip::MAX_CPUS);
	uf.set_receive_crd(Crd(caps.get(),Math::next_pow2_shift<size_t>(Hip::MAX_CPUS),Crd::OBJ_ALL));
	uf << String(name);
	CPU::current().get_pt->call(uf);

	ErrorCode res;
	uf >> res;
	if(res != E_SUCCESS)
		throw ServiceRegistryException(res);
	uf >> available;
	return caps.release();
}

void ChildManager::Portals::get_service(capsel_t) {
	ChildManager *cm = Ec::current()->get_tls<ChildManager*>(Ec::TLS_PARAM);
	UtcbFrameRef uf;
	try {
		String name;
		uf >> name;
		uf.finish_input();

		const ServiceRegistry::Service* s = cm->get_service(name);

		uf.delegate(CapRange(s->pts(),Hip::MAX_CPUS,Crd::OBJ_ALL));
		uf << E_SUCCESS << s->available();
	}
	catch(const Exception& e) {
		Syscalls::revoke(uf.get_receive_crd(),true);
		uf.clear();
		uf << e.code();
	}
}

void ChildManager::Portals::gsi(capsel_t pid) {
	ChildManager *cm = Ec::current()->get_tls<ChildManager*>(Ec::TLS_PARAM);
	UtcbFrameRef uf;
	try {
		Child *c = cm->get_child(pid);
		uint gsi;
		Gsi::Op op;
		uf >> op >> gsi;
		uf.finish_input();

		if(gsi >= Hip::MAX_GSIS)
			throw Exception(E_ARGS_INVALID);
		// make sure that just the owner can release it
		if(op == Gsi::RELEASE && !c->gsis().is_set(gsi))
			throw Exception(E_ARGS_INVALID);

		{
			UtcbFrame puf;
			if(op == Gsi::ALLOC)
				puf.set_receive_crd(Crd(c->_gsi_caps + gsi,0,Crd::OBJ_ALL));
			puf << op << gsi;
			CPU::current().gsi_pt->call(puf);
			ErrorCode res;
			puf >> res;
			if(res != E_SUCCESS)
				throw Exception(res);

			c->gsis().set(gsi,op == Gsi::ALLOC);
		}

		if(op == Gsi::ALLOC)
			uf.delegate(c->_gsi_caps + gsi);
		uf << E_SUCCESS;
	}
	catch(const Exception& e) {
		Syscalls::revoke(uf.get_receive_crd(),true);
		uf.clear();
		uf << e.code();
	}
}

void ChildManager::Portals::io(capsel_t pid) {
	ChildManager *cm = Ec::current()->get_tls<ChildManager*>(Ec::TLS_PARAM);
	UtcbFrameRef uf;
	try {
		Child *c = cm->get_child(pid);
		Ports::port_t base;
		uint count;
		Ports::Op op;
		uf >> op >> base >> count;
		uf.finish_input();

		// alloc() makes sure that nobody can free something from other childs.
		if(op == Ports::RELEASE)
			c->io().alloc(base,count);

		{
			UtcbFrame puf;
			if(op == Ports::ALLOC)
				puf.set_receive_crd(Crd(0,31,Crd::IO_ALL));
			puf << op << base << count;
			CPU::current().io_pt->call(puf);
			ErrorCode res;
			puf >> res;
			if(res != E_SUCCESS)
				throw Exception(res);
		}

		if(op == Ports::ALLOC) {
			c->io().free(base,count);
			uf.delegate(CapRange(base,count,Crd::IO_ALL));
		}
		uf << E_SUCCESS;
	}
	catch(const Exception& e) {
		Syscalls::revoke(uf.get_receive_crd(),true);
		uf.clear();
		uf << e.code();
	}
}

void ChildManager::Portals::map(capsel_t pid) {
	ChildManager *cm = Ec::current()->get_tls<ChildManager*>(Ec::TLS_PARAM);
	ScopedLock<UserSm> guard(&cm->_sm);
	UtcbFrameRef uf;
	DataSpace ds;
	int state = 0;
	try {
		Child *c = cm->get_child(pid);
		uf >> ds;
		uf.finish_input();

		Serial::get().writef("[CM=%s] Got DS: ",c->cmdline());
		ds.write(Serial::get());
		Serial::get().writef("\n");

		// map and add can throw; ensure that the state doesn't change if something throws
		bool newds = ds.sel() == ObjCap::INVALID;
		if(ds.sel() != ObjCap::INVALID) {
			bool res = cm->_dsm.join(ds);
			if(!res) {
				ds.map();
				state = 1;
				cm->_dsm.add(ds,ds.virt(),ds.size());
			}
		}
		else {
			ds.map();
			state = 1;
			cm->_dsm.add(ds,ds.virt(),ds.size());
		}
		state = 2;

		// add it to the regions of the child
		uintptr_t addr = c->reglist().find_free(ds.size());
		c->reglist().add(ds,addr,ds.perm());

		Serial::get().writef("[CM=%s] Mapped DS (%s): ",c->cmdline(),newds ? "new" : "join");
		ds.write(Serial::get());
		Serial::get().writef("\n");

		// build answer
		uf.accept_delegates();
		if(newds) {
			uf.delegate(ds.sel(),0);
			uf.delegate(ds.unmapsel(),1);
		}
		else
			uf.delegate(ds.unmapsel());
		uf << E_SUCCESS << addr << ds.size() << ds.perm() << ds.type();
	}
	catch(const Exception& e) {
		Syscalls::revoke(uf.get_receive_crd(),true);
		switch(state) {
			case 2: {
				bool destroyable = false;
				cm->_dsm.release(ds.unmapsel(),destroyable);
				if(!destroyable)
					break;
				// fall through
			}

			case 1:
				ds.unmap();
				break;
		}
		uf.clear();
		uf << e.code();
	}
}

void ChildManager::Portals::unmap(capsel_t pid) {
	ChildManager *cm = Ec::current()->get_tls<ChildManager*>(Ec::TLS_PARAM);
	ScopedLock<UserSm> guard(&cm->_sm);
	UtcbFrameRef uf;
	try {
		Child *c = cm->get_child(pid);
		// we can't trust the dataspace properties, except unmapsel
		DataSpace uds;
		uf >> uds;
		uf.finish_input();

		// destroy (decrease refs) the ds
		bool destroyable = false;
		DataSpace *ds = cm->_dsm.release(uds.unmapsel(),destroyable);
		c->reglist().remove(*ds);
		// we can only revoke the memory if there are no references anymore, because we revoke it
		// from all Pds we have delegated it to.
		if(destroyable)
			ds->unmap();
	}
	catch(const Exception& e) {
		// ignore
	}
}

void ChildManager::Portals::pf(capsel_t pid) {
	ChildManager *cm = Ec::current()->get_tls<ChildManager*>(Ec::TLS_PARAM);
	UtcbExcFrameRef uf;
	Child *c = cm->get_child(pid);
	cpu_t cpu = cm->get_cpu(pid);

	bool kill = false;
	try {
		ScopedLock<UserSm> guard(&c->_sm);
		uintptr_t pfaddr = uf->qual[1];
		unsigned error = uf->qual[0];
		uintptr_t eip = uf->rip;

		Serial::get().writef("Child '%s': Pagefault for %p @ %p on cpu %u, error=%#x\n",
				c->cmdline(),pfaddr,eip,cpu,error);

		// TODO different handlers (cow, ...)
		pfaddr &= ~(ExecEnv::PAGE_SIZE - 1);
		uintptr_t src;
		size_t size;
		uint flags = c->reglist().find(pfaddr,src,size);
		kill = !flags;
		// check if the access rights are violated
		if(flags & ChildMemory::M) {
			if((error & 0x2) && !(flags & ChildMemory::W))
				kill = true;
			if((error & 0x4) && !(flags & ChildMemory::R))
				kill = true;
		}
		if(kill) {
			uintptr_t *addr,addrs[32];
			Serial::get().writef("Child '%s': Pagefault for %p @ %p on cpu %u, error=%#x\n",
					c->cmdline(),pfaddr,eip,cpu,error);
			c->reglist().write(Serial::get());
			Serial::get().writef("Unable to resolve fault; killing Ec\n");
			ExecEnv::collect_backtrace(c->_ec->stack().virt(),uf->rbp,addrs,32);
			Serial::get().writef("Backtrace:\n");
			addr = addrs;
			while(*addr != 0) {
				Serial::get().writef("\t%p\n",*addr);
				addr++;
			}
		}
		else if(!(flags & ChildMemory::M)) {
			uint perms = flags & ChildMemory::RWX;
			// try to map the next 32 pages
			size_t msize = Math::min<size_t>(Math::round_up<size_t>(size,ExecEnv::PAGE_SIZE),32 << ExecEnv::PAGE_SHIFT);
			uf.delegate(CapRange(src >> ExecEnv::PAGE_SHIFT,msize >> ExecEnv::PAGE_SHIFT,
					Crd::MEM | (perms << 2),pfaddr >> ExecEnv::PAGE_SHIFT));
			c->reglist().map(pfaddr,msize);
			// ensure that we have the memory (if we're a subsystem this might not be true)
			// TODO this is not sufficient, in general
			UNUSED volatile int x = *reinterpret_cast<int*>(src);
		}
	}
	catch(...) {
		kill = true;
	}

	// we can't release the lock after having killed the child. thus, we do out here (it's save
	// because there can't be any running Ecs anyway since we only destroy it when there are no
	// other Ecs left)
	if(kill) {
		// let the kernel kill the Ec by causing it a pagefault in kernel-area
		uf->mtd = Mtd::RIP_LEN;
		uf->rip = ExecEnv::KERNEL_START;
		cm->destroy_child(pid);
	}
}

}
