/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <ex/Exception.h>
#include <kobj/Pd.h>
#include <kobj/GlobalEc.h>
#include <kobj/Sc.h>
#include <kobj/Pt.h>
#include <kobj/Sm.h>
#include <utcb/UtcbFrame.h>
#include <mem/RegionList.h>
#include <stream/Log.h>
#include <arch/Elf.h>
#include <ScopedPtr.h>

using namespace nul;

enum {
	MAX_CLIENTS = 32
};

class ElfException : public Exception {
public:
	ElfException(const char *msg) : Exception(msg) {
	}
};

struct Child {
	Pd *pd;
	GlobalEc *ec;
	Sc *sc;
	Pt *pf_pt;
	Pt *start_pt;
	RegionList regs;
	uintptr_t stack;
	uintptr_t utcb;
	uintptr_t hip;

	Child() : pd(), ec(), sc(), pf_pt(), start_pt(), regs(), stack(), utcb(), hip() {
	}
	~Child() {
		if(pd)
			delete pd;
		if(ec)
			delete ec;
		if(sc)
			delete sc;
		if(pf_pt)
			delete pf_pt;
		if(start_pt)
			delete start_pt;
	}
};

PORTAL static void portal_startup(cap_t pid);
PORTAL static void portal_pf(cap_t pid);
static void load_mod(uintptr_t addr,size_t size);

static size_t child = 0;
static Child *childs[MAX_CLIENTS];
static cap_t portal_caps;
static Sm *sm;

void start_childs() {
	int i = 0;
	const Hip &hip = Hip::get();
	sm = new Sm(1);
	portal_caps = CapSpace::get().allocate(MAX_CLIENTS * Hip::get().service_caps(),Hip::get().service_caps());
	for(Hip::mem_const_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		// we are the first one :)
		if(it->type == Hip_mem::MB_MODULE && i++ == 1) {
			// map the memory of the module
			UtcbFrame uf;
			uf.set_receive_crd(Crd(0,31,DESC_MEM_ALL));
			uf << CapRange(it->addr >> ExecEnv::PAGE_SHIFT,it->size,DESC_MEM_ALL);
			CPU::current().map_pt->call(uf);

			load_mod(it->addr,it->size);
		}
	}
}

static Child *get_child(cap_t pid) {
	return childs[((pid - portal_caps) / Hip::get().service_caps())];
}

static void destroy_child(cap_t pid) {
	size_t i = (pid - portal_caps) / Hip::get().service_caps();
	delete childs[i];
	childs[i] = 0;
}

static void portal_startup(cap_t pid) {
	UtcbExcFrameRef uf;
	Child *c = get_child(pid);
	assert(c);
	uf->mtd = MTD_RIP_LEN | MTD_RSP | MTD_GPR_ACDB;
	uf->eip = *reinterpret_cast<uint32_t*>(uf->esp);
	uf->esp = c->stack + (uf->esp & (ExecEnv::PAGE_SIZE - 1));
	uf->eax = c->ec->cpu();
	uf->ecx = c->hip;
	uf->edx = c->utcb;
	uf->ebx = 1;
}

static void portal_pf(cap_t pid) {
	UtcbExcFrameRef uf;
	Child *c = get_child(pid);
	assert(c);
	uintptr_t pfaddr = uf->qual[1];
	uintptr_t eip = uf->eip;

	sm->down();
	Serial::get().writef("\nPagefault for address %p @ %p\n",pfaddr,eip);

	pfaddr &= ~(ExecEnv::PAGE_SIZE - 1);
	uintptr_t src;
	uint flags = c->regs.find(pfaddr,src);
	// not found or already mapped?
	if(!flags || (flags & RegionList::M)) {
		Serial::get().writef("Unable to resolve fault; killing child\n");
		destroy_child(pid);
	}
	else {
		uint perms = flags & RegionList::RWX;
		uf.delegate(CapRange(src >> ExecEnv::PAGE_SHIFT,1,
				DESC_TYPE_MEM | (perms << 2),pfaddr >> ExecEnv::PAGE_SHIFT));
		c->regs.map(pfaddr);

		c->regs.write(Serial::get());
	}
	Serial::get().writef("\n");
	sm->up();
}

static void load_mod(uintptr_t addr,size_t size) {
	ElfEh *elf = reinterpret_cast<ElfEh*>(addr);

	// check ELF
	if(size < sizeof(ElfEh) || sizeof(ElfPh) > elf->e_phentsize || size < elf->e_phoff + elf->e_phentsize * elf->e_phnum)
		throw ElfException("Invalid ELF");
	if(!(elf->e_ident[0] == 0x7f && elf->e_ident[1] == 'E' && elf->e_ident[2] == 'L' && elf->e_ident[3] == 'F'))
		throw ElfException("Invalid signature");

	// create child
	Child *c = new Child();
	try {
		// we have to create the portals first to be able to delegate them to the new Pd
		cap_t portals = portal_caps + child * Hip::get().service_caps();
		c->pf_pt = new Pt(CPU::current().ec,portals + CapSpace::EV_PAGEFAULT,portal_pf,MTD_QUAL | MTD_RIP_LEN);
		c->start_pt = new Pt(CPU::current().ec,portals + CapSpace::EV_STARTUP,portal_startup,MTD_RSP);
		// now create Pd and pass event- and service-portals
		uint order = Util::bsr(Hip::get().service_caps());
		Log::get().writef("%u\n",order);
		c->pd = new Pd(Crd(portals,Util::bsr(Hip::get().service_caps()),DESC_CAP_ALL));
		c->ec = new GlobalEc(reinterpret_cast<GlobalEc::startup_func>(elf->e_entry),0,0,c->pd);

		// check load segments and add them to regions
		for(size_t i = 0; i < elf->e_phnum; i++) {
			ElfPh *ph = reinterpret_cast<ElfPh*>(addr + elf->e_phoff + i * elf->e_phentsize);
			if(ph->p_type != 1)
				continue;
			if(size < ph->p_offset + ph->p_filesz)
				throw ElfException("Load segment invalid");

			uint perms;
			if(ph->p_flags & PF_R)
				perms |= RegionList::R;
			if(ph->p_flags & PF_W)
				perms |= RegionList::W;
			if(ph->p_flags & PF_X)
				perms |= RegionList::X;
			c->regs.add(ph->p_vaddr,ph->p_memsz,addr + ph->p_offset,perms);
		}

		// he needs a stack and utcb
		c->stack = c->regs.find_free(ExecEnv::STACK_SIZE);
		c->regs.add(c->stack,ExecEnv::STACK_SIZE,c->ec->stack(),RegionList::RW);
		c->utcb = c->regs.find_free(ExecEnv::PAGE_SIZE);
		c->regs.add(c->utcb,ExecEnv::PAGE_SIZE,reinterpret_cast<uintptr_t>(c->ec->utcb()),RegionList::RW);
		// TODO give the child his own Hip
		c->hip = reinterpret_cast<uintptr_t>(&Hip::get());
		c->regs.add(c->hip,ExecEnv::PAGE_SIZE,c->hip,RegionList::R);

		sm->down();
		c->regs.write(Serial::get());
		sm->up();

		// start child; we have to put the child into the list before that
		childs[child++] = c;
		c->sc = new Sc(c->ec,Qpd(),c->pd);
	}
	catch(...) {
		delete c;
		childs[--child] = 0;
	}
}
