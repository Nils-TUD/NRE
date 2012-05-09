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
#include <CPU.h>

// TODO there is a case when a service is not notified that a client died (non-existent portal or something)
// TODO perhaps we can build a very small boot-task, which establishes an equal environment for all childs
// that is, it abstracts away the differences between the root-task and others, so that we can have a
// common library for all except the tiny boot-task which has its own little lib
// TODO we can't do synchronous IPC with untrusted tasks

using namespace nul;

enum {
	MAX_CLIENTS		= 32,
	MAX_SERVICES	= 32,
};

class Service {
public:
	Service(const char *name,cpu_t cpu,cap_t pt) : _name(name), _cpu(cpu), _pt(pt) {
	}

	const char *name() const {
		return _name;
	}
	cpu_t cpu() const {
		return _cpu;
	}
	cap_t pt() const {
		return _pt;
	}

private:
	const char *_name;
	cpu_t _cpu;
	cap_t _pt;
};

class ServiceRegistry {
public:
	ServiceRegistry() : _srvs() {
	}

	void reg(const Service& s) {
		for(size_t i = 0; i < MAX_SERVICES; ++i) {
			if(_srvs[i] == 0) {
				_srvs[i] = new Service(s);
				return;
			}
		}
		throw Exception("All service slots in use");
	}
	const Service& find(const char *name,cpu_t cpu) const {
		for(size_t i = 0; i < MAX_SERVICES; ++i) {
			if(_srvs[i] && _srvs[i]->cpu() == cpu && strcmp(_srvs[i]->name(),name) == 0)
				return *_srvs[i];
		}
		throw Exception("Unknown service");
	}

private:
	Service *_srvs[MAX_SERVICES];
};

class ElfException : public Exception {
public:
	ElfException(const char *msg) : Exception(msg) {
	}
};

struct Child {
	const char *cmdline;
	bool started;
	Pd *pd;
	GlobalEc *ec;
	Sc *sc;
	Pt **pts;
	size_t pt_count;
	RegionList regs;
	uintptr_t entry;
	uintptr_t stack;
	uintptr_t utcb;
	uintptr_t hip;

	Child(const char *cmdline) : cmdline(cmdline), started(), pd(), ec(), sc(), pts(), pt_count(),
			regs(), entry(), stack(), utcb(), hip() {
	}
	~Child() {
		if(pd)
			delete pd;
		if(ec)
			delete ec;
		if(sc)
			delete sc;
		for(size_t i = 0; i < pt_count; ++i)
			delete pts[i];
		delete[] pts;
	}
};

static size_t child = 0;
static Child *childs[MAX_CLIENTS];
static size_t cpu_count;
static cap_t portal_caps;
static ServiceRegistry registry;
static Sm *sm;
static LocalEc *ecs[Hip::MAX_CPUS];
static LocalEc *regecs[Hip::MAX_CPUS];

PORTAL static void portal_initcaps(cap_t pid);
PORTAL static void portal_register(cap_t pid);
PORTAL static void portal_getservice(cap_t pid);
PORTAL static void portal_map(cap_t pid);
PORTAL static void portal_startup(cap_t pid);
PORTAL static void portal_pf(cap_t pid);
static void load_mod(uintptr_t addr,size_t size,const char *cmdline);
static inline size_t portal_count() {
	return 6;
}
static inline size_t per_client_caps() {
	return Hip::get().service_caps() * cpu_count;
}

void start_childs() {
	int i = 0;
	const Hip &hip = Hip::get();
	sm = new Sm(1);
	cpu_count = hip.cpu_online_count();
	portal_caps = CapSpace::get().allocate(MAX_CLIENTS * per_client_caps(),per_client_caps());

	for(Hip::cpu_const_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled()) {
			ecs[it->id()] = new LocalEc(it->id());
			// we need a dedicated Ec for the register-portal which does always accept one capability from
			// the caller
			regecs[it->id()] = new LocalEc(it->id());
			UtcbFrameRef reguf(regecs[it->id()]->utcb());
			reguf.set_receive_crd(Crd(CapSpace::get().allocate(),0,DESC_CAP_ALL));
		}
	}

	for(Hip::mem_const_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		// we are the first one :)
		if(it->type == Hip_mem::MB_MODULE && i++ >= 1) {
			// map the memory of the module
			UtcbFrame uf;
			uf.set_receive_crd(Crd(0,31,DESC_MEM_ALL));
			uf << CapRange(it->addr >> ExecEnv::PAGE_SHIFT,it->size,DESC_MEM_ALL);
			// we assume that the cmdline does not cross pages
			if(it->aux)
				uf << CapRange(it->aux >> ExecEnv::PAGE_SHIFT,1,DESC_MEM_ALL);
			CPU::current().map_pt->call(uf);
			if(it->aux) {
				// ensure that its terminated at the end of the page
				char *end = reinterpret_cast<char*>(Util::roundup<uintptr_t>(it->aux,ExecEnv::PAGE_SIZE) - 1);
				*end = '\0';
			}

			load_mod(it->addr,it->size,reinterpret_cast<char*>(it->aux));
		}
	}
}

static cpu_t get_cpu(cap_t pid) {
	size_t off = (pid - portal_caps) % per_client_caps();
	return off / Hip::get().service_caps();
}

static Child *get_child(cap_t pid) {
	return childs[((pid - portal_caps) / per_client_caps())];
}

static void destroy_child(cap_t pid) {
	size_t i = (pid - portal_caps) / per_client_caps();
	delete childs[i];
	childs[i] = 0;
}

static void portal_initcaps(cap_t pid) {
	UtcbFrameRef uf;
	Child *c = get_child(pid);
	if(!c)
		return;

	uf.delegate(c->pd->cap(),0);
	uf.delegate(c->ec->cap(),1);
	uf.delegate(c->sc->cap(),2);
}

static void portal_register(cap_t pid) {
	UtcbFrameRef uf;
	Child *c = get_child(pid);
	if(!c)
		return;

	try {
		TypedItem cap;
		uf.get_typed(cap);
		String name;
		cpu_t cpu;
		uf >> name;
		uf >> cpu;
		registry.reg(Service(name.str(),cpu,cap.crd().cap()));

		uf.clear();
		uf.set_receive_crd(Crd(CapSpace::get().allocate(),0,DESC_CAP_ALL));
		uf << 1;
	}
	catch(Exception& e) {
		uf.clear();
		uf << 0;
	}
}

static void portal_getservice(cap_t pid) {
	UtcbFrameRef uf;
	Child *c = get_child(pid);
	cpu_t cpu = get_cpu(pid);
	if(!c)
		return;

	try {
		String name;
		uf >> name;
		Log::get().writef("Searching for '%s' on cpu %u\n",name.str(),cpu);
		const Service& s = registry.find(name.str(),cpu);

		uf.clear();
		uf.delegate(s.pt());
		uf << 1;
	}
	catch(Exception& e) {
		uf.clear();
		uf << 0;
	}
}

static void portal_map(cap_t pid) {
	UtcbFrameRef uf;
	try {
		CapRange caps;
		uf >> caps;
		uf.clear();
		uf.delegate(caps,0);
		uf << 0;
	}
	catch(const Exception& e) {
		uf.clear();
		uf << 0;
	}
}

static void portal_startup(cap_t pid) {
	UtcbExcFrameRef uf;
	Child *c = get_child(pid);
	if(!c)
		return;

	uf->mtd = MTD_RIP_LEN | MTD_RSP | MTD_GPR_ACDB;
	if(c->started) {
		uintptr_t src;
		if(!c->regs.find(uf->esp,src))
			return;
		uf->eip = *reinterpret_cast<uint32_t*>(src + (uf->esp & (ExecEnv::PAGE_SIZE - 1)));
		uf->mtd = MTD_RIP_LEN;
	}
	else {
		uf->eip = *reinterpret_cast<uint32_t*>(uf->esp);
		uf->mtd = MTD_RIP_LEN | MTD_RSP | MTD_GPR_ACDB;
		uf->esp = c->stack + (uf->esp & (ExecEnv::PAGE_SIZE - 1));
		uf->eax = c->ec->cpu();
		uf->ecx = c->hip;
		uf->edx = c->utcb;
		uf->ebx = 1;
	}
	c->started = true;
}

static void portal_pf(cap_t pid) {
	UtcbExcFrameRef uf;
	Child *c = get_child(pid);
	if(!c)
		return;

	uintptr_t pfaddr = uf->qual[1];
	uintptr_t eip = uf->eip;

	sm->down();
	Serial::get().writef("Child '%s': Pagefault for %p @ %p, error=%#x\n",c->cmdline,pfaddr,eip,uf->qual[0]);

	pfaddr &= ~(ExecEnv::PAGE_SIZE - 1);
	uintptr_t src;
	uint flags = c->regs.find(pfaddr,src);
	// not found or already mapped?
	if(!flags || (flags & RegionList::M)) {
		uintptr_t *addr,addrs[32];
		Serial::get().writef("Unable to resolve fault; killing child\n");
		ExecEnv::collect_backtrace(c->ec->stack(),uf->ebp,addrs,sizeof(addrs));
		Serial::get().writef("Backtrace:\n");
		addr = addrs;
		while(*addr != 0) {
			Serial::get().writef("\t%p\n",*addr);
			addr++;
		}
		destroy_child(pid);
	}
	else {
		uint perms = flags & RegionList::RWX;
		uf.delegate(CapRange(src >> ExecEnv::PAGE_SHIFT,1,
				DESC_TYPE_MEM | (perms << 2),pfaddr >> ExecEnv::PAGE_SHIFT));
		c->regs.map(pfaddr);
	}
	sm->up();
}

static void load_mod(uintptr_t addr,size_t size,const char *cmdline) {
	ElfEh *elf = reinterpret_cast<ElfEh*>(addr);

	// check ELF
	if(size < sizeof(ElfEh) || sizeof(ElfPh) > elf->e_phentsize || size < elf->e_phoff + elf->e_phentsize * elf->e_phnum)
		throw ElfException("Invalid ELF");
	if(!(elf->e_ident[0] == 0x7f && elf->e_ident[1] == 'E' && elf->e_ident[2] == 'L' && elf->e_ident[3] == 'F'))
		throw ElfException("Invalid signature");

	// create child
	Child *c = new Child(cmdline);
	try {
		// we have to create the portals first to be able to delegate them to the new Pd
		const size_t ptcount = portal_count();
		cap_t portals = portal_caps + child * per_client_caps();
		c->pt_count = cpu_count * ptcount;
		c->pts = new Pt*[c->pt_count];
		for(cpu_t cpu = 0; cpu < cpu_count; ++cpu) {
			size_t idx = cpu * ptcount;
			size_t off = cpu * Hip::get().service_caps();
			c->pts[idx + 0] = new Pt(ecs[cpu],portals + off + CapSpace::EV_PAGEFAULT,portal_pf,
					MTD_GPR_BSD | MTD_QUAL | MTD_RIP_LEN);
			c->pts[idx + 1] = new Pt(ecs[cpu],portals + off + CapSpace::EV_STARTUP,portal_startup,MTD_RSP);
			c->pts[idx + 2] = new Pt(ecs[cpu],portals + off + CapSpace::SRV_INIT,portal_initcaps,0);
			c->pts[idx + 3] = new Pt(regecs[cpu],portals + off + CapSpace::SRV_REGISTER,portal_register,0);
			c->pts[idx + 4] = new Pt(ecs[cpu],portals + off + CapSpace::SRV_GET,portal_getservice,0);
			c->pts[idx + 5] = new Pt(ecs[cpu],portals + off + CapSpace::SRV_MAP,portal_map,0);
		}
		// now create Pd and pass portals
		c->pd = new Pd(Crd(portals,Util::nextpow2shift(per_client_caps()),DESC_CAP_ALL));
		// TODO wrong place
		c->utcb = 0x7FFFF000;
		c->ec = new GlobalEc(reinterpret_cast<GlobalEc::startup_func>(elf->e_entry),0,0,c->pd,c->utcb);
		c->entry = elf->e_entry;

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
			if(ph->p_filesz < ph->p_memsz)
				memset(reinterpret_cast<void*>(addr + ph->p_offset + ph->p_filesz),0,ph->p_memsz - ph->p_filesz);
			c->regs.add(ph->p_vaddr,ph->p_memsz,addr + ph->p_offset,perms);
		}

		// he needs a stack
		c->stack = c->regs.find_free(ExecEnv::STACK_SIZE);
		c->regs.add(c->stack,ExecEnv::STACK_SIZE,c->ec->stack(),RegionList::RW);
		// TODO give the child his own Hip
		c->hip = reinterpret_cast<uintptr_t>(&Hip::get());
		c->regs.add(c->hip,ExecEnv::PAGE_SIZE,c->hip,RegionList::R);

		sm->down();
		Serial::get().writef("Starting client '%s'...\n",c->cmdline);
		c->regs.write(Serial::get());
		Serial::get().writef("\n");
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
