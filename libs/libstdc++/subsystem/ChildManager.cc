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
#include <util/Math.h>

namespace nul {

size_t ChildManager::_cpu_count = Hip::get().cpu_online_count();

ChildManager::ChildManager() : _child_count(), _childs(),
		_portal_caps(CapSpace::get().allocate(MAX_CHILDS * per_child_caps(),per_child_caps())),
		_dsm(), _registry(), _sm(), _regsm(0), _diesm(0), _ecs(), _regecs() {
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
	CapSpace::get().free(MAX_CHILDS * per_child_caps());
}

void ChildManager::prepare_stack(Child *c,uintptr_t &sp,uintptr_t csp) {
	/*
	 * Initial stack:
	 * +------------------+  <- top
	 * |     arguments    |
	 * |        ...       |
	 * +------------------+
	 * |         0        |
	 * +------------------+
	 * |   argv[argc-1]   |
	 * +------------------+
	 * |       ...        |
	 * +------------------+
	 * |     argv[0]      | <--\
	 * +------------------+    |
	 * |       argv       | ---/
	 * +------------------+
	 * |       argc       |
	 * +------------------+
	 */

	// first, simply copy the command-line to the stack
	const String &cmdline = c->cmdline();
	size_t len = Math::min<size_t>(MAX_CMDLINE_LEN,cmdline.length());
	char *bottom = reinterpret_cast<char*>(sp - Math::round_up(len + 1,sizeof(word_t)));
	memcpy(bottom,cmdline.str(),len + 1);

	// count number of arguments
	size_t i = 0,argc = 0;
	char *str = bottom;
	while(*str) {
		if(*str == ' ' && i > 0)
			argc++;
		else if(*str != ' ')
			i++;
		str++;
	}
	if(i > 0)
		argc++;

	word_t *ptrs = reinterpret_cast<word_t*>(bottom - sizeof(word_t) * (argc + 1));
	// store argv and argc
	*(ptrs - 1) = csp + (reinterpret_cast<word_t>(ptrs) & (ExecEnv::PAGE_SIZE - 1));
	*(ptrs - 2) = argc;
	// store stackpointer for user
	sp = csp + (reinterpret_cast<uintptr_t>(ptrs - 2) & (ExecEnv::PAGE_SIZE - 1));

	// now, walk through it, replace ' ' by '\0' and store pointers to the individual arguments
	str = bottom;
	i = 0;
	char *begin = bottom;
	while(*str) {
		if(*str == ' ' && i > 0) {
			*ptrs++ = csp + (reinterpret_cast<word_t>(begin) & (ExecEnv::PAGE_SIZE - 1));
			*str = '\0';
			i = 0;
		}
		else if(*str != ' ') {
			if(i == 0)
				begin = str;
			i++;
		}
		str++;
	}
	if(i > 0)
		*ptrs++ = csp + (reinterpret_cast<word_t>(begin) & (ExecEnv::PAGE_SIZE - 1));
	// terminate
	*ptrs++ = 0;
}

void ChildManager::load(uintptr_t addr,size_t size,const char *cmdline,uintptr_t main) {
	ElfEh *elf = reinterpret_cast<ElfEh*>(addr);

	// check ELF
	if(size < sizeof(ElfEh) || sizeof(ElfPh) > elf->e_phentsize ||
			size < elf->e_phoff + elf->e_phentsize * elf->e_phnum)
		throw ElfException(E_ELF_INVALID);
	if(!(elf->e_ident[0] == 0x7f && elf->e_ident[1] == 'E' &&
			elf->e_ident[2] == 'L' && elf->e_ident[3] == 'F'))
		throw ElfException(E_ELF_SIG);

	// create child
	Child *c = new Child(this,cmdline);
	size_t idx = free_slot();
	try {
		// we have to create the portals first to be able to delegate them to the new Pd
		capsel_t pts = _portal_caps + idx * per_child_caps();
		c->_ptcount = _cpu_count * Portals::COUNT;
		c->_pts = new Pt*[c->_ptcount];
		for(cpu_t cpu = 0; cpu < _cpu_count; ++cpu) {
			size_t idx = cpu * Portals::COUNT;
			size_t off = cpu * Hip::get().service_caps();
			c->_pts[idx + 0] = new Pt(_ecs[cpu],pts + off + CapSpace::EV_PAGEFAULT,Portals::pf,
					Mtd(Mtd::GPR_BSD | Mtd::QUAL | Mtd::RIP_LEN));
			c->_pts[idx + 1] = new Pt(_ecs[cpu],pts + off + CapSpace::EV_STARTUP,Portals::startup,
					Mtd(Mtd::RSP));
			c->_pts[idx + 2] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_INIT,Portals::init_caps,Mtd(0));
			c->_pts[idx + 3] = new Pt(_regecs[cpu],pts + off + CapSpace::SRV_REG,Portals::reg,Mtd(0));
			c->_pts[idx + 4] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_UNREG,Portals::unreg,Mtd(0));
			c->_pts[idx + 5] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_GET,Portals::get_service,Mtd(0));
			c->_pts[idx + 6] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_IO,Portals::io,Mtd(0));
			c->_pts[idx + 7] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_GSI,Portals::gsi,Mtd(0));
			c->_pts[idx + 8] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_MAP,Portals::map,Mtd(0));
			c->_pts[idx + 9] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_UNMAP,Portals::unmap,Mtd(0));
		}
		// now create Pd and pass portals
		c->_pd = new Pd(Crd(pts,Math::next_pow2_shift(per_child_caps()),Crd::OBJ_ALL));
		c->_entry = elf->e_entry;
		c->_main = main;

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

			size_t dssize = Math::round_up<size_t>(ph->p_memsz,ExecEnv::PAGE_SIZE);
			// TODO leak, if reglist().add throws
			const DataSpace &ds = _dsm.create(
					DataSpaceDesc(dssize,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RWX));
			// TODO actually it would be better to do that later
			memcpy(reinterpret_cast<void*>(ds.virt()),
					reinterpret_cast<void*>(addr + ph->p_offset),ph->p_filesz);
			memset(reinterpret_cast<void*>(ds.virt() + ph->p_filesz),0,ph->p_memsz - ph->p_filesz);
			c->reglist().add(ds.desc(),ph->p_vaddr,perms,ds.sel());
		}

		// utcb
		c->_utcb = c->reglist().find_free(Utcb::SIZE);
		// just reserve the virtual memory with no permissions; it will not be requested
		c->reglist().add(c->_utcb,Utcb::SIZE,0,0);
		c->_ec = new GlobalEc(reinterpret_cast<GlobalEc::startup_func>(elf->e_entry),0,0,c->_pd,c->_utcb);

		// he needs a stack; use guards around it
		c->_stack = c->reglist().find_free(ExecEnv::STACK_SIZE + ExecEnv::PAGE_SIZE * 2);
		c->reglist().add(c->stack(),ExecEnv::PAGE_SIZE,0,0);
		c->reglist().add(c->stack() + ExecEnv::PAGE_SIZE,ExecEnv::STACK_SIZE,c->_ec->stack(),ChildMemory::RW);
		c->reglist().add(c->stack() + ExecEnv::PAGE_SIZE * 2,ExecEnv::PAGE_SIZE,0,0);
		c->_stack += ExecEnv::PAGE_SIZE;

		// and a HIP
		{
			const DataSpace &ds = _dsm.create(
					DataSpaceDesc(ExecEnv::PAGE_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW));
			c->_hip = c->reglist().find_free(ExecEnv::PAGE_SIZE);
			memcpy(reinterpret_cast<void*>(ds.virt()),&Hip::get(),ExecEnv::PAGE_SIZE);
			// TODO we need to adjust the hip
			c->reglist().add(ds.desc(),c->_hip,ChildMemory::R,ds.sel());
		}

		Serial::get() << "Starting client '" << c->cmdline() << "'...\n";
		Serial::get() << *c << "\n";

		// start child; we have to put the child into the list before that
		_childs[idx] = c;
		c->_sc = new Sc(c->_ec,Qpd(),c->_pd);
		c->_sc->start();
	}
	catch(...) {
		delete c;
		_childs[idx] = 0;
		throw;
	}

	_child_count++;

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
			prepare_stack(c,uf->rsp,c->stack());
			// the bit indicates that its not the root-task
			// TODO not nice
	#ifdef __i386__
			uf->rax = (1 << 31) | c->_ec->cpu();
	#else
			uf->rdi = (1 << 31) | c->_ec->cpu();
	#endif
			uf->rsi = c->_main;
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
		capsel_t cap = uf.get_delegated(uf.get_receive_crd().order()).offset();
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
	if(!CPU::current().get_pt)
		throw ServiceRegistryException(E_NOT_FOUND);

	UtcbFrame uf;
	ScopedCapSels caps(Hip::MAX_CPUS,Hip::MAX_CPUS);
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

		ScopedLock<UserSm> guard(&c->_sm);
		{
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
		}
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

		ScopedLock<UserSm> guard(&c->_sm);
		{
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
	UtcbFrameRef uf;
	try {
		capsel_t sel = 0;
		DataSpaceDesc desc;
		DataSpace::RequestType type;
		Child *c = cm->get_child(pid);
		uf >> type >> desc;
		if(type == DataSpace::JOIN)
			sel = uf.get_translated(0).offset();
		uf.finish_input();

		ScopedLock<UserSm> guard(&c->_sm);
		uintptr_t addr = 0;
		if(desc.type() == DataSpaceDesc::VIRTUAL) {
			addr = c->reglist().find_free(desc.size());
			c->reglist().add(addr,desc.size(),desc.perm(),0);
			uf << E_SUCCESS << DataSpaceDesc(desc.size(),desc.type(),desc.perm(),0,addr);
		}
		else {
			// create it or attach to the existing dataspace
			const DataSpace &ds = type == DataSpace::JOIN ? cm->_dsm.join(desc,sel) : cm->_dsm.create(desc);

			// add it to the regions of the child
			try {
				// use guard-pages around the dataspace
				addr = c->reglist().find_free(ds.size() + ExecEnv::PAGE_SIZE * 2);
				c->reglist().add(addr,ExecEnv::PAGE_SIZE,0,0);
				c->reglist().add(ds.desc(),addr + ExecEnv::PAGE_SIZE,ds.perm(),ds.unmapsel());
				c->reglist().add(addr + ExecEnv::PAGE_SIZE + ds.size(),ExecEnv::PAGE_SIZE,0,0);
				addr += ExecEnv::PAGE_SIZE;
			}
			catch(...) {
				cm->_dsm.release(desc,ds.unmapsel());
				throw;
			}

			// build answer
			if(type == DataSpace::CREATE) {
				uf.delegate(ds.sel(),0);
				uf.delegate(ds.unmapsel(),1);
			}
			else {
				uf.accept_delegates();
				uf.delegate(ds.unmapsel());
			}
			uf << E_SUCCESS << DataSpaceDesc(ds.size(),ds.type(),ds.perm(),ds.virt(),addr);
		}
	}
	catch(const Exception& e) {
		Syscalls::revoke(uf.get_receive_crd(),true);
		uf.clear();
		uf << e.code();
	}
}

void ChildManager::Portals::unmap(capsel_t pid) {
	ChildManager *cm = Ec::current()->get_tls<ChildManager*>(Ec::TLS_PARAM);
	UtcbFrameRef uf;
	try {
		Child *c = cm->get_child(pid);
		// we can't trust the dataspace properties, except unmapsel
		DataSpaceDesc desc;
		DataSpace::RequestType type;
		capsel_t sel = 0;
		uf >> type >> desc;
		if(desc.type() != DataSpaceDesc::VIRTUAL)
			sel = uf.get_translated(0).offset();
		uf.finish_input();

		ScopedLock<UserSm> guard(&c->_sm);
		if(desc.type() == DataSpaceDesc::VIRTUAL)
			c->reglist().remove(desc.virt(),desc.size());
		else {
			// destroy (decrease refs) the ds
			cm->_dsm.release(desc,sel);
			DataSpaceDesc childdesc = c->reglist().remove(sel);
			c->reglist().remove(childdesc.virt() - ExecEnv::PAGE_SIZE,ExecEnv::PAGE_SIZE);
			c->reglist().remove(childdesc.virt() + childdesc.size(),ExecEnv::PAGE_SIZE);
		}
	}
	catch(const Exception& e) {
		Syscalls::revoke(uf.get_receive_crd(),true);
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

		//Serial::get().writef("Child '%s': Pagefault for %p @ %p on cpu %u, error=%#x\n",
		//		c->cmdline().str(),pfaddr,eip,cpu,error);

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
					c->cmdline().str(),pfaddr,eip,cpu,error);
			//Serial::get() << c->reglist();
			Serial::get().writef("Unable to resolve fault; killing Ec\n");
			ExecEnv::collect_backtrace(c->_ec->stack(),uf->rbp,addrs,32);
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
			// TODO perhaps we could find the dataspace, that belongs to this address and use this
			// one to notify the parent that he should map it?
			UNUSED volatile int x = *reinterpret_cast<int*>(src);
		}
		else {
			Serial::get().writef("Child '%s': Pagefault for %p @ %p on cpu %u, error=%#x\n",
					c->cmdline().str(),pfaddr,eip,cpu,error);
			Serial::get() << "Page already mapped. See regionlist:\n";
			Serial::get() << c->reglist();
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
