#include <kobj/GlobalEc.h>
#include <kobj/LocalEc.h>
#include <kobj/Sc.h>
#include <kobj/Sm.h>
#include <Syscalls.h>
#include <String.h>
#include <Utcb.h>
#include <Hip.h>
#include <Log.h>
#include <CPU.h>
#include <exception>
#include <assert.h>

#define PAGE_SIZE	0x1000

typedef void (*thread_func)();

extern "C" void abort();
extern "C" int start(cpu_t cpu,Utcb *utcb);
static void portal_startup(unsigned pid) __attribute__((regparm(1)));
static void portal_map(unsigned pid) __attribute__((regparm(1)));
static void mythread();

static Log *log;
static Sm *sm;

void verbose_terminate() {
	try {
		throw;
	}
	catch(const Exception& e) {
		e.print(*log);
	}
	catch(...) {
		log->print("Uncatched, unknown exception\n");
	}
	abort();
}

int start(cpu_t cpu,Utcb *utcb) {
	const Hip &hip = Hip::get();
	GlobalEc::set_current(new GlobalEc(utcb,hip.cfg_exc + 1,cpu,new Pd(hip.cfg_exc + 0,true)));

	for(Hip::cpu_const_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled()) {
			CPU &cpu = CPU::get(it->id());
			cpu.id = it->id();
			cpu.ec = new LocalEc(cpu.id,hip.event_caps() * (cpu.id + 1));
			cpu.map_pt = cpu.ec->create_portal(portal_map,cpu.id);
			cpu.ec->create_portal_for(0x1E,portal_startup,MTD_RSP);
		}
	}

	//Pd::current()->io().delegate(Pd::current(),0x3F8,5);
	//Pd::current()->mem().delegate(Pd::current(),0x100,16);

	// put Crd's in our utcb as untyped items to send them to the map-portal
	unsigned *item = utcb->msg;
	item[1] = MAP_HBIT;
	item[0] = Crd(0x3F8,3,DESC_IO_ALL).value();
	// prepare dst-utcb (our)
	utcb->head.untyped = 2;
	utcb->head.crd = Crd(0, 20, DESC_IO_ALL).value();
	Syscalls::call(CPU::get(cpu).map_pt);

	log = new Log();

	item = utcb->msg;
	item[1] = 0x200 << 12 | MAP_HBIT;
	item[0] = Crd(0x100,4,DESC_MEM_ALL).value();
	utcb->head.untyped = 2;
	utcb->head.crd = Crd(0, 31, DESC_MEM_ALL).value();
	Syscalls::call(CPU::get(cpu).map_pt);

	while(1);

	std::set_terminate(verbose_terminate);

	*(char*)0x200000 = 4;
	*(char*)(0x200000 + PAGE_SIZE * 16 - 1) = 4;
	assert(*(char*)0x200000 == 4);
	//assert(*(char*)0x200000 == 5);

	log->print("SEL: %u, EXC: %u, VMI: %u, GSI: %u\n",
			hip.cfg_cap,hip.cfg_exc,hip.cfg_vm,hip.cfg_gsi);
	log->print("Memory:\n");
	for(Hip::mem_const_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		log->print("\taddr=%#Lx, size=%#Lx, type=%d\n",it->addr,it->size,it->type);
	}

	log->print("CPUs:\n");
	for(Hip::cpu_const_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled())
			log->print("\tpackage=%u, core=%u thread=%u, flags=%u\n",it->package,it->core,it->thread,it->flags);
	}

	try {
		sm = new Sm(1);

		new Sc(new GlobalEc(mythread,0),Qpd());
		new Sc(new GlobalEc(mythread,1),Qpd(1,100));
		new Sc(new GlobalEc(mythread,1),Qpd(1,1000));
	}
	catch(const SyscallException& e) {
		e.print(*log);
	}

	mythread();
	return 0;
}

static void portal_startup(unsigned pid) {
	Utcb *utcb = Ec::current()->utcb();
	utcb->mtd = MTD_RIP_LEN;
	utcb->eip = *reinterpret_cast<uint32_t*>(utcb->esp);
}

static void portal_map(unsigned pid) {
	Utcb *utcb = Ec::current()->utcb();
	utcb->set_header(0,utcb->head.untyped / 2);
	memmove(utcb->item_start(),utcb->msg,sizeof(unsigned) * utcb->head.typed * 2);
}

static void mythread() {
	Utcb *utcb = Ec::current()->utcb();
	unsigned *item = utcb->msg;
	item[1] = (0x200 + Ec::current()->cap()) << 12 | MAP_HBIT;
	item[0] = Crd(0x100 + Ec::current()->cap(),4,DESC_MEM_ALL).value();
	utcb->head.untyped = 2;
	utcb->head.crd = Crd(0, 20, DESC_MEM_ALL).value();
	Syscalls::call(CPU::get(Ec::current()->cpu()).map_pt);

	while(1)
		;

	while(1) {
		sm->down();
		log->print("I am Ec %u, running on CPU %u\n",Ec::current()->cap(),Ec::current()->cpu());
		sm->up();
	}
}
