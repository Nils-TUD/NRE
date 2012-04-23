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

struct Client {
	Pd *pd;
	GlobalEc *ec;
	Sc *sc;
	Pt *pf_pt;
	Pt *start_pt;
	RegionList regs;
	uintptr_t stack;

	Client() : pd(), ec(), sc(), pf_pt(), start_pt(), regs(), stack() {
	}
	~Client() {
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
static size_t client = 0;
static Client *clients[MAX_CLIENTS];
static cap_t portal_caps;

void start_clients() {
	int i = 0;
	const Hip &hip = Hip::get();
	portal_caps = CapSpace::get().allocate(MAX_CLIENTS * hip.cfg_exc,hip.cfg_exc);
	for(Hip::mem_const_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		// we are the first one :)
		if(it->type == Hip_mem::MB_MODULE && i++ == 1) {
			map_mem(it->addr,it->size);
			load_mod(it->addr,it->size);
		}
	}
}

static Client *get_client(cap_t pid) {
	return clients[((pid - portal_caps) / Hip::get().cfg_exc)];
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

static void portal_startup(cap_t pid) {
	Client *c = get_client(pid);
	UtcbExc *utcb = Ec::current()->exc_utcb();
	utcb->mtd = MTD_RIP_LEN | MTD_RSP;
	utcb->eip = *reinterpret_cast<uint32_t*>(utcb->esp);
	utcb->esp = c->stack + (utcb->esp & ExecEnv::PAGE_SIZE);
}

static void portal_pf(cap_t pid) {
	UtcbExc *utcb = Ec::current()->exc_utcb();
	log->print("Page fault @ %p, esp=%p\n",utcb->eip,utcb->esp);
}

static void load_mod(uintptr_t addr,size_t size) {
	ElfEh *elf = reinterpret_cast<ElfEh*>(addr);

	// check ELF
	if(size < sizeof(ElfEh) || sizeof(ElfPh) > elf->e_phentsize || size < elf->e_phoff + elf->e_phentsize * elf->e_phnum)
		throw ElfException("Invalid ELF");
	if(!(elf->e_ident[0] == 0x7f && elf->e_ident[1] == 'E' && elf->e_ident[2] == 'L' && elf->e_ident[3] == 'F'))
		throw ElfException("Invalid signature");

	// create client
	ScopedPtr<Client> c(new Client());
	// we have to create the portals first to be able to delegate them to the new Pd
	cap_t portals = portal_caps + client * Hip::get().cfg_exc;
	// TODO constants for exceptions
	c->pf_pt = new Pt(CPU::current().ec,portals + 0xe,portal_pf,MTD_ALL);
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
		c->regs.add(ph->p_vaddr,ph->p_memsz,reinterpret_cast<const void*>(addr + ph->p_offset),perms);
	}

	// he needs a stack and utcb
	c->stack = c->regs.find_free(ExecEnv::STACK_SIZE);
	c->regs.add(c->stack,ExecEnv::STACK_SIZE,reinterpret_cast<const void*>(c->ec->stack()),RegionList::RW);
	uintptr_t utcb = c->regs.find_free(ExecEnv::PAGE_SIZE);
	c->regs.add(utcb,ExecEnv::PAGE_SIZE,c->ec->utcb(),RegionList::RW);

	c->regs.print(*log);

	// start client
	c->sc = new Sc(c->ec,Qpd(),c->pd);
	clients[client++] = c.release();
}
