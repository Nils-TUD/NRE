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
#include <arch/Elf.h>
#include <ScopedPtr.h>
#include <Log.h>

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
static void map_mem(uintptr_t phys,size_t size);
static void load_mod(uintptr_t addr,size_t size);

extern Log *log;
static size_t child = 0;
static Child *childs[MAX_CLIENTS];
static cap_t portal_caps;
static Sm *sm;

void start_childs() {
	int i = 0;
	const Hip &hip = Hip::get();
	sm = new Sm(1);
	portal_caps = CapSpace::get().allocate(MAX_CLIENTS * hip.cfg_exc,hip.cfg_exc);
	for(Hip::mem_const_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		// we are the first one :)
		if(it->type == Hip_mem::MB_MODULE && i++ == 1) {
			map_mem(it->addr,it->size);
			load_mod(it->addr,it->size);
		}
	}
}

static Child *get_child(cap_t pid) {
	return childs[((pid - portal_caps) / Hip::get().cfg_exc)];
}

static void destroy_child(cap_t pid) {
	size_t i = (pid - portal_caps) / Hip::get().cfg_exc;
	delete childs[i];
	childs[i] = 0;
}

static void map_mem(uintptr_t phys,size_t size) {
	UtcbFrame uf;
	uintptr_t frame = phys >> ExecEnv::PAGE_SHIFT;
	uf.set_receive_crd(Crd(0, 31, DESC_MEM_ALL));
	size = (size + ExecEnv::PAGE_SIZE - 1) & ~(ExecEnv::PAGE_SIZE - 1);
	while(size > 0) {
		uint minshift = Util::minshift(phys,size);
		uf << DelItem(Crd(frame,minshift - ExecEnv::PAGE_SHIFT,DESC_MEM_ALL),DelItem::FROM_HV,frame);
		frame += 1 << (minshift - ExecEnv::PAGE_SHIFT);
		phys += 1 << minshift;
		size -= 1 << minshift;
	}
	CPU::current().map_pt->call();
}

static void delegate_mem(UtcbFrameRef &uf,uintptr_t dest,uintptr_t src,size_t size,uint perms) {
	const int shift = ExecEnv::PAGE_SHIFT;
	size = (size + ExecEnv::PAGE_SIZE - 1) & ~(ExecEnv::PAGE_SIZE - 1);
	while(size > 0) {
		uint minshift = Util::minshift(src | dest,size);
		uf.add_typed(DelItem(Crd(src >> shift,minshift - shift,DESC_TYPE_MEM | (perms << 2)),0,dest >> shift));
		size_t amount = 1 << minshift;
		src += amount;
		dest += amount;
		size -= amount;
	}
}

static void portal_startup(cap_t pid) {
	Child *c = get_child(pid);
	assert(c);
	UtcbExc *utcb = Ec::current()->exc_utcb();
	utcb->mtd = MTD_RIP_LEN | MTD_RSP | MTD_GPR_ACDB;
	utcb->eip = *reinterpret_cast<uint32_t*>(utcb->esp);
	utcb->esp = c->stack + (utcb->esp & (ExecEnv::PAGE_SIZE - 1));
	utcb->eax = c->ec->cpu();
	utcb->ecx = c->hip;
	utcb->edx = c->utcb;
	utcb->ebx = 1;
}

static void portal_pf(cap_t pid) {
	Child *c = get_child(pid);
	assert(c);
	UtcbExc *utcb = Ec::current()->exc_utcb();
	uintptr_t pfaddr = utcb->qual[1];
	uintptr_t eip = utcb->eip;

	sm->down();
	log->print("\nPagefault for address %p @ %p\n",pfaddr,eip);

	UtcbFrameRef uf;
	uf.reset();

	pfaddr &= ~(ExecEnv::PAGE_SIZE - 1);
	uintptr_t src;
	uint flags = c->regs.find(pfaddr,&src);
	// not found or already mapped?
	if(!flags || (flags & RegionList::M)) {
		log->print("Unable to resolve fault; killing child\n");
		destroy_child(pid);
	}
	else {
		uint perms = flags & RegionList::RWX;
		delegate_mem(uf,pfaddr & ~(ExecEnv::PAGE_SIZE - 1),src,ExecEnv::PAGE_SIZE,perms);
		c->regs.map(pfaddr);

		c->regs.print(*log);
	}
	log->print("\n");
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
		cap_t portals = portal_caps + child * Hip::get().cfg_exc;
		// TODO constants for exceptions
		c->pf_pt = new Pt(CPU::current().ec,portals + 0xe,portal_pf,MTD_QUAL | MTD_RIP_LEN);
		c->start_pt = new Pt(CPU::current().ec,portals + 0x1e,portal_startup,MTD_RSP);
		// now create Pd and pass exception-portals
		c->pd = new Pd(Crd(portals,5,DESC_CAP_ALL));
		c->ec = new GlobalEc(reinterpret_cast<GlobalEc::startup_func>(elf->e_entry),0,0,c->pd);

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
		c->regs.print(*log);
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
