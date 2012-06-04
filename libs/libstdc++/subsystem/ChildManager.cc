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
#include <mem/Memory.h>
#include <cap/Caps.h>
#include <arch/Elf.h>

namespace nul {

size_t ChildManager::_cpu_count = Hip::get().cpu_online_count();

ChildManager::ChildManager() : _child(), _childs(),
		_portal_caps(CapSpace::get().allocate(MAX_CHILDS * per_child_caps(),per_child_caps())),
		_dsm(), _registry(), _sm(), _ecs(), _capecs() {
	const Hip& hip = Hip::get();
	for(Hip::cpu_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled()) {
			{
				_ecs[it->id()] = new LocalEc(it->id());
				size_t tls = _ecs[it->id()]->create_tls();
				assert(tls == 0);
				_ecs[it->id()]->set_tls(tls,this);
			}
			UtcbFrameRef defuf(_ecs[it->id()]->utcb());
			defuf.set_translate_crd(Crd(0,31,DESC_CAP_ALL));
			{
				// we need a dedicated Ec for the register-portal which does always accept one
				// capability from the caller
				_capecs[it->id()] = new LocalEc(it->id());
				size_t tls = _capecs[it->id()]->create_tls();
				assert(tls == 0);
				_capecs[it->id()]->set_tls(tls,this);
			}
			UtcbFrameRef capuf(_capecs[it->id()]->utcb());
			capuf.set_receive_crd(Crd(CapSpace::get().allocate(),0,DESC_CAP_ALL));
			capuf.set_translate_crd(Crd(0,31,DESC_CAP_ALL));
		}
	}
}

ChildManager::~ChildManager() {
	for(size_t i = 0; i < MAX_CHILDS; ++i)
		delete _childs[i];
	for(size_t i = 0; i < Hip::MAX_CPUS; ++i) {
		delete _ecs[i];
		delete _capecs[i];
	}
}

void ChildManager::load(uintptr_t addr,size_t size,const char *cmdline) {
	ElfEh *elf = reinterpret_cast<ElfEh*>(addr);

	// check ELF
	if(size < sizeof(ElfEh) || sizeof(ElfPh) > elf->e_phentsize ||
			size < elf->e_phoff + elf->e_phentsize * elf->e_phnum)
		throw ElfException("Invalid ELF");
	if(!(elf->e_ident[0] == 0x7f && elf->e_ident[1] == 'E' &&
			elf->e_ident[2] == 'L' && elf->e_ident[3] == 'F'))
		throw ElfException("Invalid signature");

	// create child
	Child *c = new Child(cmdline);
	try {
		// we have to create the portals first to be able to delegate them to the new Pd
		capsel_t pts = _portal_caps + _child * per_child_caps();
		c->_ptcount = _cpu_count * Portals::COUNT;
		c->_pts = new Pt*[c->_ptcount];
		c->_smcount = _cpu_count;
		c->_sms = new Sm*[_cpu_count];
		c->_waitcount = _cpu_count;
		c->_waits = new uint[_cpu_count];
		for(cpu_t cpu = 0; cpu < _cpu_count; ++cpu) {
			size_t idx = cpu * Portals::COUNT;
			size_t off = cpu * Hip::get().service_caps();
			c->_pts[idx + 0] = new Pt(_ecs[cpu],pts + off + CapSpace::EV_PAGEFAULT,Portals::pf,
					MTD_GPR_BSD | MTD_QUAL | MTD_RIP_LEN);
			c->_pts[idx + 1] = new Pt(_ecs[cpu],pts + off + CapSpace::EV_STARTUP,Portals::startup,MTD_RSP);
			c->_pts[idx + 2] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_INIT,Portals::initcaps,0);
			c->_pts[idx + 3] = new Pt(_capecs[cpu],pts + off + CapSpace::SRV_REG,Portals::reg,0);
			c->_pts[idx + 4] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_UNREG,Portals::unreg,0);
			c->_pts[idx + 5] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_GET,Portals::getservice,0);
			c->_pts[idx + 6] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_ALLOCIO,Portals::allocio,0);
			c->_pts[idx + 7] = new Pt(_capecs[cpu],pts + off + CapSpace::SRV_MAP,Portals::map,0);
			c->_pts[idx + 8] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_UNMAP,Portals::unmap,0);
			c->_sms[cpu] = new Sm(pts + off + CapSpace::SM_SERVICE,0U);
			c->_waits[cpu] = 0;
		}
		// now create Pd and pass portals
		c->_pd = new Pd(Crd(pts,Util::nextpow2shift(per_child_caps()),DESC_CAP_ALL));
		// TODO wrong place
		c->_utcb = 0x7FFFF000;
		c->_ec = new GlobalEc(reinterpret_cast<GlobalEc::startup_func>(elf->e_entry),
				0,0,c->_pd,c->_utcb);
		c->_entry = elf->e_entry;

		// check load segments and add them to regions
		for(size_t i = 0; i < elf->e_phnum; i++) {
			ElfPh *ph = reinterpret_cast<ElfPh*>(addr + elf->e_phoff + i * elf->e_phentsize);
			if(reinterpret_cast<uintptr_t>(ph) + sizeof(ElfPh) > addr + size)
				throw ElfException("Load segment pointer invalid");
			if(ph->p_type != 1)
				continue;
			if(size < ph->p_offset + ph->p_filesz)
				throw ElfException("Load segment invalid");

			uint perms = 0;
			if(ph->p_flags & PF_R)
				perms |= RegionList::R;
			if(ph->p_flags & PF_W)
				perms |= RegionList::W;
			if(ph->p_flags & PF_X)
				perms |= RegionList::X;

			DataSpace ds(Util::roundup<size_t>(ph->p_memsz,ExecEnv::PAGE_SIZE),DataSpace::ANONYMOUS,RegionList::RWX);
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
		c->reglist().add(c->stack(),ExecEnv::STACK_SIZE,c->_ec->stack().virt(),RegionList::RW);
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
	}
	catch(...) {
		delete c;
		_childs[--_child] = 0;
		throw;
	}
}

void ChildManager::Portals::startup(capsel_t pid) {
	ChildManager *cm = Ec::current()->get_tls<ChildManager>(0);
	UtcbExcFrameRef uf;
	Child *c = cm->get_child(pid);
	if(!c)
		return;

	if(c->_started) {
		uintptr_t src;
		size_t size;
		if(!c->reglist().find(uf->rsp,src,size))
			return;
		uf->rip = *reinterpret_cast<word_t*>(src + (uf->rsp & (ExecEnv::PAGE_SIZE - 1)) + sizeof(word_t));
		uf->mtd = MTD_RIP_LEN;
		Serial::get().writef("### Start @ %p\n",uf->rip);
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
		uf->mtd = MTD_RIP_LEN | MTD_RSP | MTD_GPR_ACDB | MTD_GPR_BSD;
		c->_started = true;
	}
}

void ChildManager::Portals::initcaps(capsel_t pid) {
	ChildManager *cm = Ec::current()->get_tls<ChildManager>(0);
	UtcbFrameRef uf;
	Child *c = cm->get_child(pid);
	if(!c)
		return;

	// we can't give the child the cap for e.g. the Pd when creating the Pd. therefore the child
	// grabs them afterwards with this portal
	uf.delegate(c->_pd->sel(),0);
	uf.delegate(c->_ec->sel(),1);
	uf.delegate(c->_sc->sel(),2);
}

void ChildManager::Portals::reg(capsel_t pid) {
	ChildManager *cm = Ec::current()->get_tls<ChildManager>(0);
	UtcbFrameRef uf;
	Child *c = cm->get_child(pid);
	if(!c)
		return;

	try {
		TypedItem cap;
		uf.get_typed(cap);
		String name;
		cpu_t cpu;
		uf >> name;
		uf >> cpu;
		{
			ScopedLock<UserSm> guard(&cm->_sm);
			cm->registry().reg(ServiceRegistry::Service(c,name,cpu,cap.crd().cap()));
			cm->notify_childs(cpu);
		}

		uf.clear();
		uf.set_receive_crd(Crd(CapSpace::get().allocate(),0,DESC_CAP_ALL));
		// TODO error/success codes?
		uf << 1;
	}
	catch(Exception& e) {
		uf.clear();
		uf << 0;
	}
}

void ChildManager::Portals::unreg(capsel_t pid) {
	ChildManager *cm = Ec::current()->get_tls<ChildManager>(0);
	UtcbFrameRef uf;
	Child *c = cm->get_child(pid);
	if(!c)
		return;

	try {
		String name;
		cpu_t cpu;
		uf >> name;
		uf >> cpu;
		{
			ScopedLock<UserSm> guard(&cm->_sm);
			cm->registry().unreg(c,name,cpu);
		}

		uf.clear();
		uf << 1;
	}
	catch(Exception& e) {
		uf.clear();
		uf << 0;
	}
}

// TODO code-duplication; we already have that in the Client class
static capsel_t get_parent_service(const char *name) {
	if(!CPU::current().get_pt)
		throw Exception("Service not found");
	UtcbFrame uf;
	uf.set_receive_crd(Crd(CapSpace::get().allocate(),0,DESC_CAP_ALL));
	uf.clear();
	uf << String(name);
	CPU::current().get_pt->call(uf);

	TypedItem ti;
	uf.get_typed(ti);
	return ti.crd().cap();
}

void ChildManager::Portals::getservice(capsel_t pid) {
	ChildManager *cm = Ec::current()->get_tls<ChildManager>(0);
	UtcbFrameRef uf;
	Child *c = cm->get_child(pid);
	cpu_t cpu = cm->get_cpu(pid);
	if(!c)
		return;

	try {
		String name;
		uf >> name;
		{
			ScopedLock<UserSm> guard(&cm->_sm);
			c->_waits[cpu]++;
			const ServiceRegistry::Service* s = cm->registry().find(name,cpu);
			if(!s) {
				capsel_t sel = get_parent_service(name.str());
				s = cm->registry().reg(ServiceRegistry::Service(0,name,cpu,sel));
			}
			c->_waits[cpu]--;

			uf.clear();
			uf.delegate(s->pt());
			uf << 1;
		}
	}
	catch(Exception& e) {
		uf.clear();
		uf << 0;
	}
}

void ChildManager::Portals::allocio(capsel_t) {
	UtcbFrameRef uf;
	// TODO
	try {
		CapRange caps;
		uf >> caps;
		uf.clear();
		uf.delegate(caps,DelItem::FROM_HV);
		uf << 0;
	}
	catch(const Exception& e) {
		uf.clear();
		uf << 0;
	}
}

void ChildManager::Portals::map(capsel_t pid) {
	ChildManager *cm = Ec::current()->get_tls<ChildManager>(0);
	Child *c = cm->get_child(pid);
	if(!c)
		return;

	ScopedLock<UserSm> guard(&cm->_sm);
	bool created = false;
	UtcbFrameRef uf;
	DataSpace ds;
	try {
		uf >> ds;
		// create and add can throw; ensure that the state doesn't change if something throws
		cm->_dsm.create(ds);
		created = true;
		uintptr_t addr = c->reglist().find_free(ds.size());
		c->reglist().add(ds,addr,ds.perm());

		Serial::get().writef("\n### %s: Created dataspace @ %p with %zu bytes\n",c->cmdline(),addr,ds.size());
		c->reglist().write(Serial::get());
		Serial::get().writef("\n");

		uf.clear();
		uf.delegate(ds.sel(),0);
		uf.delegate(ds.unmapsel(),1);
		uf.set_receive_crd(Crd(CapSpace::get().allocate(),0,DESC_CAP_ALL));
		uf << addr << ds.size() << ds.perm() << ds.type();
	}
	catch(const Exception& e) {
		// TODO revoke ds-cap?
		// TODO that just decreases the references
		if(created)
			cm->_dsm.destroy(ds.unmapsel());
		uf.clear();
		uf << 0;
	}
}

static void revoke_mem(uintptr_t addr,size_t size) {
	size_t count = size >> ExecEnv::PAGE_SHIFT;
	uintptr_t start = addr >> ExecEnv::PAGE_SHIFT;
	while(count > 0) {
		uint minshift = Util::minshift(start,count);
		Syscalls::revoke(Crd(start,minshift,DESC_MEM_ALL),false);
		start += 1 << minshift;
		count -= 1 << minshift;
	}
}

void ChildManager::Portals::unmap(capsel_t pid) {
	ChildManager *cm = Ec::current()->get_tls<ChildManager>(0);
	Child *c = cm->get_child(pid);
	if(!c)
		return;

	ScopedLock<UserSm> guard(&cm->_sm);
	UtcbFrameRef uf;
	try {
		// we can't trust the dataspace properties, except unmapsel
		DataSpace uds;
		uf >> uds;
		// destroy (decrease refs) the ds
		DataSpace *ds = cm->_dsm.destroy(uds.unmapsel());
		// we can only revoke the memory if there are no references anymore, because we revoke it
		// from all Pds we have delegated it to. thus, it does make no sense to remove it from the
		// list before revoking the mem, because that would be really unpredictable.
		if(ds) {
			c->reglist().remove(*ds);
			revoke_mem(ds->virt(),ds->size());
			ds->unmap();
		}
		uf.clear();
		uf << 0;
	}
	catch(const Exception& e) {
		uf.clear();
		uf << 0;
	}
}

void ChildManager::Portals::pf(capsel_t pid) {
	ChildManager *cm = Ec::current()->get_tls<ChildManager>(0);
	UtcbExcFrameRef uf;
	Child *c = cm->get_child(pid);
	cpu_t cpu = cm->get_cpu(pid);
	if(!c)
		return;

	bool kill = false;
	{
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
		if(flags & RegionList::M) {
			if((error & 0x2) && !(flags & RegionList::W))
				kill = true;
			if((error & 0x4) && !(flags & RegionList::R))
				kill = true;
		}
		if(kill) {
			uintptr_t *addr,addrs[32];
			Serial::get().writef("Child '%s': Pagefault for %p @ %p on cpu %u, error=%#x\n",
					c->cmdline(),pfaddr,eip,cpu,error);
			c->reglist().write(Serial::get());
			Serial::get().writef("Unable to resolve fault; killing child\n");
			ExecEnv::collect_backtrace(c->_ec->stack().virt(),uf->rbp,addrs,32);
			Serial::get().writef("Backtrace:\n");
			addr = addrs;
			while(*addr != 0) {
				Serial::get().writef("\t%p\n",*addr);
				addr++;
			}
		}
		else if(!(flags & RegionList::M)) {
			uint perms = flags & RegionList::RWX;
			// try to map the next 32 pages
			size_t msize = Util::min<size_t>(Util::roundup<size_t>(size,ExecEnv::PAGE_SIZE),32 << ExecEnv::PAGE_SHIFT);
			uf.delegate(CapRange(src >> ExecEnv::PAGE_SHIFT,msize >> ExecEnv::PAGE_SHIFT,
					DESC_TYPE_MEM | (perms << 2),pfaddr >> ExecEnv::PAGE_SHIFT));
			c->reglist().map(pfaddr,msize);
			// ensure that we have the memory (if we're a subsystem this might not be true)
			// TODO this is not sufficient, in general
			UNUSED volatile int x = *reinterpret_cast<int*>(src);
		}
	}

	// we can't release the lock after having killed the child. thus, we do out here (it's save
	// because there can't be any running Ecs anyway since we only destroy it when there are no
	// other Ecs left)
	if(kill) {
		// let the kernel kill the Ec by causing it a pagefault in kernel-area
		uf->mtd = MTD_RIP_LEN;
		uf->rip = ExecEnv::KERNEL_START;
		cm->destroy_child(pid);
	}
}

}
