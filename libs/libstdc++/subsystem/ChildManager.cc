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
#include <arch/Elf.h>

namespace nul {

size_t ChildManager::_cpu_count = Hip::get().cpu_online_count();

ChildManager::ChildManager() : _child(), _childs(),
		_portal_caps(CapSpace::get().allocate(MAX_CHILDS * per_child_caps(),per_child_caps())),
		_registry(), _sm(), _ecs(), _regecs() {
	const Hip& hip = Hip::get();
	for(Hip::cpu_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled()) {
			_ecs[it->id()] = new LocalEc(this,it->id());
			// we need a dedicated Ec for the register-portal which does always accept one
			// capability from the caller
			_regecs[it->id()] = new LocalEc(this,it->id());
			UtcbFrameRef reguf(_regecs[it->id()]->utcb());
			reguf.set_receive_crd(Crd(CapSpace::get().allocate(),0,DESC_CAP_ALL));
		}
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
		throw ElfException("Invalid ELF");
	if(!(elf->e_ident[0] == 0x7f && elf->e_ident[1] == 'E' &&
			elf->e_ident[2] == 'L' && elf->e_ident[3] == 'F'))
		throw ElfException("Invalid signature");

	// create child
	Child *c = new Child(cmdline);
	try {
		// we have to create the portals first to be able to delegate them to the new Pd
		cap_t pts = _portal_caps + _child * per_child_caps();
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
			c->_pts[idx + 3] = new Pt(_regecs[cpu],pts + off + CapSpace::SRV_REG,Portals::reg,0);
			c->_pts[idx + 4] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_UNREG,Portals::unreg,0);
			c->_pts[idx + 5] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_GET,Portals::getservice,0);
			c->_pts[idx + 6] = new Pt(_ecs[cpu],pts + off + CapSpace::SRV_MAP,Portals::map,0);
			c->_sms[cpu] = new Sm(pts + off + CapSpace::SM_SERVICE,0U);
			c->_waits[cpu] = 0;
		}
		// now create Pd and pass portals
		c->_pd = new Pd(Crd(pts,Util::nextpow2shift(per_child_caps()),DESC_CAP_ALL));
		// TODO wrong place
		c->_utcb = 0x7FFFF000;
		c->_ec = new GlobalEc(reinterpret_cast<GlobalEc::startup_func>(elf->e_entry),
				0,0,0,c->_pd,c->_utcb);
		c->_entry = elf->e_entry;

		// check load segments and add them to regions
		for(size_t i = 0; i < elf->e_phnum; i++) {
			ElfPh *ph = reinterpret_cast<ElfPh*>(addr + elf->e_phoff + i * elf->e_phentsize);
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
			// TODO actually it would be better to do that later
			if(ph->p_filesz < ph->p_memsz) {
				memset(reinterpret_cast<void*>(addr + ph->p_offset + ph->p_filesz),0,
						ph->p_memsz - ph->p_filesz);
			}
			c->reglist().add(ph->p_vaddr,ph->p_memsz,addr + ph->p_offset,perms);
		}

		// he needs a stack
		c->_stack = c->reglist().find_free(ExecEnv::STACK_SIZE);
		c->reglist().add(c->stack(),ExecEnv::STACK_SIZE,c->_ec->stack(),RegionList::RW);
		// TODO give the child his own Hip
		c->_hip = reinterpret_cast<uintptr_t>(&Hip::get());
		c->reglist().add(c->hip(),ExecEnv::PAGE_SIZE,c->hip(),RegionList::R);

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
	}
}

void ChildManager::Portals::startup(cap_t pid,void *tls) {
	ChildManager *cm = reinterpret_cast<ChildManager*>(tls);
	UtcbExcFrameRef uf;
	Child *c = cm->get_child(pid);
	if(!c)
		return;

	if(c->_started) {
		uintptr_t src;
		if(!c->reglist().find(uf->esp,src))
			return;
		uf->eip = *reinterpret_cast<uint32_t*>(src + (uf->esp & (ExecEnv::PAGE_SIZE - 1)));
		uf->mtd = MTD_RIP_LEN;
	}
	else {
		uf->eip = *reinterpret_cast<uint32_t*>(uf->esp);
		uf->esp = c->stack() + (uf->esp & (ExecEnv::PAGE_SIZE - 1));
		// the bit indicates that its not the root-task
		uf->eax = (1 << 31) | c->_ec->cpu();
		uf->ecx = c->hip();
		uf->edx = c->utcb();
		uf->mtd = MTD_RIP_LEN | MTD_RSP | MTD_GPR_ACDB;
		c->_started = true;
	}
}

void ChildManager::Portals::initcaps(cap_t pid,void *tls) {
	ChildManager *cm = reinterpret_cast<ChildManager*>(tls);
	UtcbFrameRef uf;
	Child *c = cm->get_child(pid);
	if(!c)
		return;

	// we can't give the child the cap for e.g. the Pd when creating the Pd. therefore the child
	// grabs them afterwards with this portal
	uf.delegate(c->_pd->cap(),0);
	uf.delegate(c->_ec->cap(),1);
	uf.delegate(c->_sc->cap(),2);
}

void ChildManager::Portals::reg(cap_t pid,void *tls) {
	ChildManager *cm = reinterpret_cast<ChildManager*>(tls);
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
			cm->registry().reg(ServiceRegistry::Service(c,name.str(),cpu,cap.crd().cap()));
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

void ChildManager::Portals::unreg(cap_t pid,void *tls) {
	ChildManager *cm = reinterpret_cast<ChildManager*>(tls);
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
			cm->registry().unreg(c,name.str(),cpu);
		}

		uf.clear();
		uf << 1;
	}
	catch(Exception& e) {
		uf.clear();
		uf << 0;
	}
}

void ChildManager::Portals::getservice(cap_t pid,void *tls) {
	ChildManager *cm = reinterpret_cast<ChildManager*>(tls);
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
			const ServiceRegistry::Service& s = cm->registry().find(name.str(),cpu);
			c->_waits[cpu]--;

			uf.clear();
			uf.delegate(s.pt());
			uf << 1;
		}
	}
	catch(Exception& e) {
		uf.clear();
		uf << 0;
	}
}

void ChildManager::Portals::map(cap_t,void *) {
	UtcbFrameRef uf;
	// TODO we have to manage the resources!
	try {
		CapRange caps;
		uf >> caps;
		uf.clear();
		uf.delegate(caps);
		uf << 0;
	}
	catch(const Exception& e) {
		uf.clear();
		uf << 0;
	}
}

void ChildManager::Portals::pf(cap_t pid,void *tls) {
	ChildManager *cm = reinterpret_cast<ChildManager*>(tls);
	static UserSm sm;
	UtcbExcFrameRef uf;
	Child *c = cm->get_child(pid);
	cpu_t cpu = cm->get_cpu(pid);
	if(!c)
		return;

	ScopedLock<UserSm> guard(&c->_sm);
	uintptr_t pfaddr = uf->qual[1];
	unsigned error = uf->qual[0];
	uintptr_t eip = uf->eip;

	// TODO different handlers (cow, ...)
	pfaddr &= ~(ExecEnv::PAGE_SIZE - 1);
	uintptr_t src;
	uint flags = c->reglist().find(pfaddr,src);
	bool kill = !flags;
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
		Serial::get().writef("Unable to resolve fault; killing child\n");
		ExecEnv::collect_backtrace(c->_ec->stack(),uf->ebp,addrs,sizeof(addrs));
		Serial::get().writef("Backtrace:\n");
		addr = addrs;
		while(*addr != 0) {
			Serial::get().writef("\t%p\n",*addr);
			addr++;
		}
		cm->destroy_child(pid);
	}
	else if(!(flags & RegionList::M)) {
		uint perms = flags & RegionList::RWX;
		uf.delegate(CapRange(src >> ExecEnv::PAGE_SHIFT,1,
				DESC_TYPE_MEM | (perms << 2),pfaddr >> ExecEnv::PAGE_SHIFT));
		c->reglist().map(pfaddr);
	}
}

}
