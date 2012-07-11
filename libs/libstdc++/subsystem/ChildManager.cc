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

#include <subsystem/ChildManager.h>
#include <stream/Serial.h>
#include <kobj/Gsi.h>
#include <kobj/Ports.h>
#include <cap/Caps.h>
#include <arch/Elf.h>
#include <util/Math.h>
#include <Logging.h>

namespace nre {

ChildManager::ChildManager() : _child_count(), _childs(),
		_portal_caps(CapSpace::get().allocate(MAX_CHILDS * per_child_caps(),per_child_caps())),
		_dsm(), _registry(), _sm(), _switchsm(), _regsm(0), _diesm(0), _ecs(), _regecs() {
	_ecs = new LocalThread*[CPU::count()];
	_regecs = new LocalThread*[CPU::count()];
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it) {
		LocalThread **ecs[] = {_ecs,_regecs};
		for(size_t i = 0; i < ARRAY_SIZE(ecs); ++i) {
			ecs[i][it->log_id()] = new LocalThread(it->log_id());
			ecs[i][it->log_id()]->set_tls(Thread::TLS_PARAM,this);
		}

		UtcbFrameRef defuf(_ecs[it->log_id()]->utcb());
		defuf.accept_translates();
		defuf.accept_delegates(0);

		UtcbFrameRef reguf(_regecs[it->log_id()]->utcb());
		reguf.accept_delegates(Math::next_pow2_shift<size_t>(CPU::count()));
	}
}

ChildManager::~ChildManager() {
	for(size_t i = 0; i < CPU::count(); ++i)
		delete _childs[i];
	for(size_t i = 0; i < CPU::count(); ++i) {
		delete _ecs[i];
		delete _regecs[i];
	}
	delete[] _ecs;
	delete[] _regecs;
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
		c->_ptcount = CPU::count() * Portals::COUNT;
		c->_pts = new Pt*[c->_ptcount];
		for(cpu_t cpu = 0; cpu < CPU::count(); ++cpu) {
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
			c->_pts[idx + 8] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_DS,Portals::dataspace,Mtd(0));
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
		c->_ec = new GlobalThread(reinterpret_cast<GlobalThread::startup_func>(elf->e_entry),0,0,c->_pd,c->_utcb);

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

		LOG(Logging::CHILD_CREATE,Serial::get() << "Starting client '" << c->cmdline() << "'...\n");
		LOG(Logging::CHILD_CREATE,Serial::get() << *c << "\n");

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
		LOG(Logging::CHILD_CREATE,Serial::get() << "Waiting for '" << name << "'...\n");
		while(_registry.find(name) == 0)
			_regsm.down();
		LOG(Logging::CHILD_CREATE,Serial::get() << "Found '" << name << "'!\n");
	}
}

void ChildManager::Portals::startup(capsel_t pid) {
	ChildManager *cm = Thread::current()->get_tls<ChildManager*>(Thread::TLS_PARAM);
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
		// let the kernel kill the Thread
		uf->rip = ExecEnv::KERNEL_START;
		uf->mtd = Mtd::RIP_LEN;
	}
}

void ChildManager::Portals::init_caps(capsel_t pid) {
	ChildManager *cm = Thread::current()->get_tls<ChildManager*>(Thread::TLS_PARAM);
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
		Syscalls::revoke(uf.delegation_window(),true);
		uf.clear();
		uf << e.code();
	}
}

void ChildManager::Portals::reg(capsel_t pid) {
	ChildManager *cm = Thread::current()->get_tls<ChildManager*>(Thread::TLS_PARAM);
	UtcbFrameRef uf;
	try {
		Child *c = cm->get_child(pid);
		String name;
		BitField<Hip::MAX_CPUS> available;
		capsel_t cap = uf.get_delegated(uf.delegation_window().order()).offset();
		uf >> name;
		uf >> available;
		uf.finish_input();

		LOG(Logging::SERVICES,Serial::get() << "Child '" << c->cmdline() << "' regs " << name << "\n");
		cm->reg_service(c,cap,name,available);

		uf.accept_delegates();
		uf << E_SUCCESS;
	}
	catch(const Exception& e) {
		Syscalls::revoke(uf.delegation_window(),true);
		uf.clear();
		uf << e.code();
	}
}

void ChildManager::Portals::unreg(capsel_t pid) {
	ChildManager *cm = Thread::current()->get_tls<ChildManager*>(Thread::TLS_PARAM);
	UtcbFrameRef uf;
	try {
		Child *c = cm->get_child(pid);
		String name;
		uf >> name;
		uf.finish_input();

		LOG(Logging::SERVICES,Serial::get() << "Child '" << c->cmdline() << "' unregs " << name << "\n");
		cm->unreg_service(c,name);

		uf << E_SUCCESS;
	}
	catch(const Exception& e) {
		Syscalls::revoke(uf.delegation_window(),true);
		uf.clear();
		uf << e.code();
	}
}

// TODO code-duplication; we already have that in the Session class
capsel_t ChildManager::get_parent_service(const char *name,BitField<Hip::MAX_CPUS> &available) {
	if(!CPU::current().get_pt)
		throw ServiceRegistryException(E_NOT_FOUND);

	UtcbFrame uf;
	ScopedCapSels caps(CPU::count(),CPU::count());
	uf.delegation_window(Crd(caps.get(),Math::next_pow2_shift<size_t>(CPU::count()),Crd::OBJ_ALL));
	uf << String(name);
	CPU::current().get_pt->call(uf);

	ErrorCode res;
	uf >> res;
	if(res != E_SUCCESS)
		throw ServiceRegistryException(res);
	uf >> available;
	return caps.release();
}

void ChildManager::Portals::get_service(capsel_t pid) {
	ChildManager *cm = Thread::current()->get_tls<ChildManager*>(Thread::TLS_PARAM);
	UtcbFrameRef uf;
	try {
		Child *c = cm->get_child(pid);
		String name;
		uf >> name;
		uf.finish_input();

		LOG(Logging::SERVICES,Serial::get() << "Child '" << c->cmdline() << "' gets " << name << "\n");
		const ServiceRegistry::Service* s = cm->get_service(name);

		uf.delegate(CapRange(s->pts(),CPU::count(),Crd::OBJ_ALL));
		uf << E_SUCCESS << s->available();
	}
	catch(const Exception& e) {
		Syscalls::revoke(uf.delegation_window(),true);
		uf.clear();
		uf << e.code();
	}
}

void ChildManager::Portals::gsi(capsel_t pid) {
	ChildManager *cm = Thread::current()->get_tls<ChildManager*>(Thread::TLS_PARAM);
	UtcbFrameRef uf;
	try {
		Child *c = cm->get_child(pid);
		uint gsi;
		void *pcicfg = 0;
		Gsi::Op op;
		uf >> op >> gsi;
		if(op == Gsi::ALLOC)
			uf >> pcicfg;
		uf.finish_input();

		capsel_t cap = 0;
		{
			ScopedLock<UserSm> guard(&c->_sm);
			if(op == Gsi::ALLOC) {
				LOG(Logging::RESOURCES,Serial::get().writef("Child '%s' allocates GSI %u (PCI %p)\n",
							c->cmdline().str(),gsi,pcicfg));
			}
			else {
				LOG(Logging::RESOURCES,Serial::get().writef("Child '%s' releases GSI %u\n",
							c->cmdline().str(),gsi));
			}

			if(gsi >= Hip::MAX_GSIS)
				throw Exception(E_ARGS_INVALID);
			// make sure that just the owner can release it
			if(op == Gsi::RELEASE && !c->gsis().is_set(gsi))
				throw Exception(E_ARGS_INVALID);
			if(c->_gsi_next == Hip::MAX_GSIS)
				throw Exception(E_CAPACITY);

			{
				UtcbFrame puf;
				puf << op << gsi;
				if(op == Gsi::ALLOC) {
					puf << pcicfg;
					cap = c->_gsi_next++;
					puf.delegation_window(Crd(c->_gsi_caps + cap,0,Crd::OBJ_ALL));
				}
				CPU::current().gsi_pt->call(puf);
				ErrorCode res;
				puf >> res;
				if(res != E_SUCCESS)
					throw Exception(res);
				if(op == Gsi::ALLOC)
					puf >> gsi;
				c->gsis().set(gsi,op == Gsi::ALLOC);
			}
		}

		uf << E_SUCCESS;
		if(op == Gsi::ALLOC) {
			uf << gsi;
			uf.delegate(c->_gsi_caps + cap);
		}
	}
	catch(const Exception& e) {
		Syscalls::revoke(uf.delegation_window(),true);
		uf.clear();
		uf << e.code();
	}
}

void ChildManager::Portals::io(capsel_t pid) {
	ChildManager *cm = Thread::current()->get_tls<ChildManager*>(Thread::TLS_PARAM);
	UtcbFrameRef uf;
	try {
		Child *c = cm->get_child(pid);
		Ports::port_t base;
		uint count;
		Ports::Op op;
		uf >> op >> base >> count;
		uf.finish_input();

		{
			ScopedLock<UserSm> guard(&c->_sm);
			if(op == Ports::ALLOC) {
				LOG(Logging::RESOURCES,Serial::get().writef("Child '%s' allocates ports %#x..%#x\n",
							c->cmdline().str(),base,base + count - 1));
			}
			else {
				LOG(Logging::RESOURCES,Serial::get().writef("Child '%s' releases ports %#x..%#x\n",
							c->cmdline().str(),base,base + count - 1));
			}

			// alloc() makes sure that nobody can free something from other childs.
			if(op == Ports::RELEASE)
				c->io().alloc(base,count);

			{
				UtcbFrame puf;
				if(op == Ports::ALLOC)
					puf.delegation_window(Crd(0,31,Crd::IO_ALL));
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
		Syscalls::revoke(uf.delegation_window(),true);
		uf.clear();
		uf << e.code();
	}
}

void ChildManager::map(UtcbFrameRef &uf,Child *c,DataSpace::RequestType type) {
	capsel_t sel = 0;
	DataSpaceDesc desc;
	uf >> desc;
	if(type == DataSpace::JOIN)
		sel = uf.get_translated(0).offset();
	uf.finish_input();

	ScopedLock<UserSm> guard(&c->_sm);
	uintptr_t addr = 0;
	if(desc.type() == DataSpaceDesc::VIRTUAL) {
		addr = c->reglist().find_free(desc.size());
		c->reglist().add(addr,desc.size(),desc.perm(),0);
		desc = DataSpaceDesc(desc.size(),desc.type(),desc.perm(),0,addr);
		LOG(Logging::DATASPACES,
				Serial::get() << "Child '" << c->cmdline() << "' allocated virtual ds:\n\t" << desc << "\n");
		uf << E_SUCCESS << desc;
	}
	else {
		// create it or attach to the existing dataspace
		const DataSpace &ds = type == DataSpace::JOIN ? _dsm.join(desc,sel) : _dsm.create(desc);

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
			_dsm.release(desc,ds.unmapsel());
			throw;
		}

		// build answer
		if(type == DataSpace::CREATE) {
			LOG(Logging::DATASPACES,
					Serial::get() << "Child '" << c->cmdline() << "' created:\n\t" << ds << "\n");
			uf.delegate(ds.sel(),0);
			uf.delegate(ds.unmapsel(),1);
		}
		else {
			LOG(Logging::DATASPACES,
					Serial::get() << "Child '" << c->cmdline() << "' joined:\n\t" << ds << "\n");
			uf.accept_delegates();
			uf.delegate(ds.unmapsel());
		}
		uf << E_SUCCESS << DataSpaceDesc(ds.size(),ds.type(),ds.perm(),ds.phys(),addr,ds.virt());
	}
}

// TODO especially this usecase shows that ChildMemory is a really bad datastructure for this
// way too much linear searches and so on
static void remap(Child *ch,DataSpaceDesc &src,DataSpaceDesc &dst,
		uintptr_t srcorg,uintptr_t dstorg,capsel_t srcsel,capsel_t dstsel) {
	// remove them from child-regions
	if(srcsel != ObjCap::INVALID)
		ch->reglist().remove(srcsel);
	if(dstsel != ObjCap::INVALID)
		ch->reglist().remove(dstsel);

	// swap origins (add expects them in virt)
	uintptr_t vsrc = src.virt();
	uintptr_t vdst = dst.virt();
	src.virt(dstorg);
	dst.virt(srcorg);
	// and map them again; the next pagefault will handle the rest
	if(srcsel != ObjCap::INVALID)
		ch->reglist().add(src,vsrc,src.perm(),srcsel);
	if(dstsel != ObjCap::INVALID)
		ch->reglist().add(dst,vdst,dst.perm(),dstsel);
}

void ChildManager::switch_to(UtcbFrameRef &uf,Child *c) {
	capsel_t srcsel = uf.get_translated(0).offset();
	capsel_t dstsel = uf.get_translated(0).offset();
	uf.finish_input();

	{
		// note that we need another lock here since it may also involve childs of c (c may have
		// delegated it and if they cause a pagefault during this operation, we might get mixed
		// results)
		ScopedLock<UserSm> guard_switch(&_switchsm);

		uintptr_t srcorg,dstorg;
		{
			// first do the stuff for the child that requested the switch
			ScopedLock<UserSm> guard_regs(&c->_sm);
			DataSpaceDesc src,dst;
			bool found_src = c->reglist().find(srcsel,src);
			bool found_dst = c->reglist().find(dstsel,dst);
			LOG(Logging::DATASPACES,Serial::get() << "Child '" << c->cmdline()
						<< "' switches:\n\t" << src << "\n\t" << dst << "\n");
			if(!found_src || !found_dst)
				throw Exception(E_ARGS_INVALID);
			if(src.size() != dst.size())
				throw Exception(E_ARGS_INVALID);

			// first revoke the memory to prevent further accesses
			CapRange(src.origin() >> ExecEnv::PAGE_SHIFT,
					src.size() >> ExecEnv::PAGE_SHIFT,Crd::MEM_ALL).revoke(false);
			CapRange(dst.origin() >> ExecEnv::PAGE_SHIFT,
					dst.size() >> ExecEnv::PAGE_SHIFT,Crd::MEM_ALL).revoke(false);
			// we have to reset the last pf information here, because of the revoke. otherwise it
			// can happen that last time CPU X caused the last fault and this time, CPU X causes
			// the second fault (the first one will handle it and the second one will find it already
			// mapped). therefore, it might be the same address and same CPU again which caused an
			// "already-mapped" fault again and thus, it would be killed.
			c->_last_fault_addr = 0;
			c->_last_fault_cpu = 0;
			// now copy the content
			memcpy(reinterpret_cast<char*>(dst.origin()),reinterpret_cast<char*>(src.origin()),src.size());
			// change mapping
			srcorg = src.origin();
			dstorg = dst.origin();
			remap(c,src,dst,srcorg,dstorg,srcsel,dstsel);
		}

		// now change the mapping for all other childs that have one of these dataspaces
		for(size_t x = 0,i = 0; i < MAX_CHILDS && x < _child_count; ++i) {
			if(_childs[i] == 0 || _childs[i] == c)
				continue;

			Child *ch = _childs[i];
			ScopedLock<UserSm> guard_regs(&ch->_sm);
			DataSpaceDesc chsrc,chdst;
			bool found_src = ch->reglist().find(srcsel,chsrc);
			bool found_dst = ch->reglist().find(dstsel,chdst);
			if(!found_src && !found_dst)
				continue;

			remap(ch,chsrc,chdst,srcorg,dstorg,found_src ? srcsel : ObjCap::INVALID,
					found_dst ? dstsel : ObjCap::INVALID);
			x++;
		}

		// now swap the origins also in the dataspace-manager (otherwise clients that join
		// afterwards will receive the wrong location)
		_dsm.swap(srcsel,dstsel);
	}

	uf << E_SUCCESS;
}

void ChildManager::unmap(UtcbFrameRef &uf,Child *c) {
	capsel_t sel = 0;
	DataSpaceDesc desc;
	uf >> desc;
	if(desc.type() != DataSpaceDesc::VIRTUAL)
		sel = uf.get_translated(0).offset();
	uf.finish_input();

	ScopedLock<UserSm> guard(&c->_sm);
	if(desc.type() == DataSpaceDesc::VIRTUAL) {
		LOG(Logging::DATASPACES,Serial::get() << "Child '" << c->cmdline()
				<< "' destroys virtual ds " << desc << "\n");
		c->reglist().remove(desc.virt(),desc.size());
	}
	else {
		LOG(Logging::DATASPACES,Serial::get() << "Child '" << c->cmdline()
				<< "' destroys " << sel << ": " << desc << "\n");
		// destroy (decrease refs) the ds
		_dsm.release(desc,sel);
		DataSpaceDesc childdesc = c->reglist().remove(sel);
		c->reglist().remove(childdesc.virt() - ExecEnv::PAGE_SIZE,ExecEnv::PAGE_SIZE);
		c->reglist().remove(childdesc.virt() + childdesc.size(),ExecEnv::PAGE_SIZE);
	}
	uf << E_SUCCESS;
}

void ChildManager::Portals::dataspace(capsel_t pid) {
	ChildManager *cm = Thread::current()->get_tls<ChildManager*>(Thread::TLS_PARAM);
	UtcbFrameRef uf;
	try {
		DataSpace::RequestType type;
		Child *c = cm->get_child(pid);
		uf >> type;

		switch(type) {
			case DataSpace::CREATE:
			case DataSpace::JOIN:
				cm->map(uf,c,type);
				break;

			case DataSpace::SWITCH_TO:
				cm->switch_to(uf,c);
				break;

			case DataSpace::DESTROY:
				cm->unmap(uf,c);
				break;

			case DataSpace::SHARE:
				throw Exception(E_ARGS_INVALID);
				break;
		}
	}
	catch(const Exception& e) {
		Syscalls::revoke(uf.delegation_window(),true);
		uf.clear();
		uf << e.code();
	}
}

void ChildManager::Portals::pf(capsel_t pid) {
	ChildManager *cm = Thread::current()->get_tls<ChildManager*>(Thread::TLS_PARAM);
	UtcbExcFrameRef uf;
	Child *c = cm->get_child(pid);
	cpu_t cpu = cm->get_cpu(pid);

	bool kill = false;
	try {
		ScopedLock<UserSm> guard_switch(&cm->_switchsm);
		ScopedLock<UserSm> guard_regs(&c->_sm);
		uintptr_t pfaddr = uf->qual[1];
		unsigned error = uf->qual[0];
		uintptr_t eip = uf->rip;

		LOG(Logging::PFS,Serial::get().writef("Child '%s': Pagefault for %p @ %p on cpu %u, error=%#x\n",
				c->cmdline().str(),pfaddr,eip,cpu,error));

		// TODO different handlers (cow, ...)
		pfaddr &= ~(ExecEnv::PAGE_SIZE - 1);
		uintptr_t src;
		size_t size;
		bool remap = false;
		uint flags = c->reglist().find(pfaddr,src,size);
		kill = !flags;
		// check if the access rights are violated
		if(flags & ChildMemory::M) {
			if((error & 0x2) && !(flags & ChildMemory::W))
				kill = true;
			if((error & 0x4) && !(flags & ChildMemory::R))
				kill = true;
		}

		// is the page already mapped (may be ok if two cpus accessed the page at the same time)
		if(!kill && (flags & ChildMemory::M)) {
			// first check if our parent has unmapped the memory
			Crd res = Syscalls::lookup(Crd(src >> ExecEnv::PAGE_SHIFT,0,Crd::MEM));
			// if so, remap it
			if(res.is_null())
				remap = true;
			// same fault for same cpu again?
			else if(pfaddr == c->_last_fault_addr && cpu == c->_last_fault_cpu) {
				LOG(Logging::CHILD_KILL,Serial::get().writef(
						"Child '%s': Caused fault for %p on cpu %u twice. Killing Thread\n",
						c->cmdline().str(),pfaddr,CPU::get(cpu).phys_id()));
				kill = true;
			}
			else {
				LOG(Logging::PFS,Serial::get().writef(
						"Child '%s': Pagefault for %p @ %p on cpu %u, error=%#x (page already mapped)\n",
						c->cmdline().str(),pfaddr,eip,CPU::get(cpu).phys_id(),error));
				LOG(Logging::PFS_DETAIL,Serial::get() << "See regionlist:\n" << c->reglist());
				c->_last_fault_addr = pfaddr;
				c->_last_fault_cpu = cpu;
			}
		}

		if(kill) {
			uintptr_t *addr,addrs[32];
			LOG(Logging::CHILD_KILL,Serial::get().writef(
					"Child '%s': Pagefault for %p @ %p on cpu %u, bp=%p, error=%#x\n",
					c->cmdline().str(),pfaddr,eip,CPU::get(cpu).phys_id(),uf->rbp,error));
			LOG(Logging::CHILD_KILL,Serial::get() << c->reglist());
			LOG(Logging::CHILD_KILL,Serial::get().writef("Unable to resolve fault; killing Thread\n"));
			ExecEnv::collect_backtrace(c->_ec->stack(),uf->rbp,addrs,32);
			LOG(Logging::CHILD_KILL,Serial::get().writef("Backtrace:\n"));
			addr = addrs;
			while(*addr != 0) {
				LOG(Logging::CHILD_KILL,Serial::get().writef("\t%p\n",*addr));
				addr++;
			}
		}
		else if(remap || !(flags & ChildMemory::M)) {
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
	}
	catch(...) {
		kill = true;
	}

	// we can't release the lock after having killed the child. thus, we do out here (it's save
	// because there can't be any running Ecs anyway since we only destroy it when there are no
	// other Ecs left)
	if(kill) {
		// let the kernel kill the Thread by causing it a pagefault in kernel-area
		uf->mtd = Mtd::RIP_LEN;
		uf->rip = ExecEnv::KERNEL_START;
		cm->destroy_child(pid);
	}
}

}
