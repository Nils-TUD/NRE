#include <Syscalls.h>
#include <String.h>
#include <Utcb.h>
#include <Hip.h>
#include <Log.h>
#include <exception>
#include <assert.h>

#define PAGE_SIZE	0x1000

typedef void (*thread_func)(void*);

enum {
	PD_ROOT	= 32,
	MIN_SHIFT = 12,
	MAX_STACKS = 4
};

extern "C" void abort();
extern "C" void idc_reply_and_wait_fast(void*);
extern "C" int start(cpu_t cpu,Utcb *utcb,Hip *hip);
static cap_t create_ec(void (*func)(void*),cpu_t cpunr,Syscalls::ECType type,unsigned exbase);
static void portal_startup(unsigned pid,void *tls,Utcb *utcb) __attribute__((regparm(1)));
static void portal_map(unsigned pid,void *tls,Utcb *utcb) __attribute__((regparm(1)));
static void mythread(void *utcb);

//static Log log;

static void *stacks[MAX_STACKS][PAGE_SIZE / sizeof(void*)] __attribute__((aligned(PAGE_SIZE)));
static size_t stack = 0;

static uintptr_t utcb_addr = 0x1000000;
static cap_t cur_cap = 3;
static cap_t map_pt;
static Log *log;

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

int start(cpu_t cpu,Utcb *utcb,Hip *hip) {
	cap_t echo_ec0 = create_ec(idc_reply_and_wait_fast,0,Syscalls::EC_WORKER,0x20);
	map_pt = cur_cap++;
	Syscalls::create_pt(map_pt,echo_ec0,reinterpret_cast<uintptr_t>(portal_map),0,PD_ROOT);

	//Pd::current()->io().delegate(Pd::current(),0x3F8,5);
	//Pd::current()->mem().delegate(Pd::current(),0x100,16);

	// put Crd's in our utcb as untyped items to send them to the map-portal
	unsigned *item = utcb->msg;
	item[1] = MAP_HBIT;
	item[0] = Crd(0x3F8,3,DESC_IO_ALL).value();
	// prepare dst-utcb (our)
	utcb->head.untyped = 2;
	utcb->head.crd = Crd(0, 20, DESC_IO_ALL).value();
	Syscalls::call(map_pt);

	log = new Log();

	item = utcb->msg;
	item[1] = 0x200 << 12 | MAP_HBIT;
	item[0] = Crd(0x100,4,DESC_MEM_ALL).value();
	utcb->head.untyped = 2;
	utcb->head.crd = Crd(0, 20, DESC_MEM_ALL).value();
	Syscalls::call(map_pt);

	std::set_terminate(verbose_terminate);

	*(char*)0x200000 = 4;
	*(char*)(0x200000 + PAGE_SIZE * 16 - 1) = 4;
	assert(*(char*)0x200000 == 4);
	assert(*(char*)0x200000 == 5);

	log->print("Memory:\n");
	for(Hip::mem_const_iterator it = hip->mem_begin(); it != hip->mem_end(); ++it) {
		log->print("\taddr=%#Lx, size=%#Lx, type=%d\n",it->addr,it->size,it->type);
	}

	log->print("CPUs:\n");
	for(Hip::cpu_const_iterator it = hip->cpu_begin(); it != hip->cpu_end(); ++it) {
		if(it->enabled())
			log->print("\tpackage=%u, core=%u thread=%u, flags=%u\n",it->package,it->core,it->thread,it->flags);
	}

	try {
		cap_t echo_ec1 = create_ec(idc_reply_and_wait_fast,1,Syscalls::EC_WORKER,0);
		Syscalls::create_pt(0x1E,echo_ec1,reinterpret_cast<uintptr_t>(portal_startup),MTD_RSP,PD_ROOT);

		cap_t ec = create_ec(mythread,1,Syscalls::EC_GENERAL,0);
		Syscalls::create_sc(cur_cap++,ec,Qpd(),1,PD_ROOT);
		Syscalls::create_sc(cur_cap++,ec,Qpd(),1,PD_ROOT);
	}
	catch(const SyscallException& e) {
		e.print(*log);
	}

	while(1)
		;
	return 0;
}

static cap_t create_ec(thread_func func,cpu_t cpunr,Syscalls::ECType type,unsigned exbase) {
	Utcb *utcb = reinterpret_cast<Utcb*>(utcb_addr);
	cap_t cap = cur_cap;
	unsigned stack_top = sizeof(stacks[stack]) / sizeof(void*);
	stacks[stack][--stack_top] = utcb; // push UTCB -- needed for myutcb()
	stacks[stack][--stack_top] = reinterpret_cast<void*>(0xDEAD);
	stacks[stack][--stack_top] = utcb; // push UTCB -- as function parameter
	stacks[stack][--stack_top] = 0; // tls
	stacks[stack][--stack_top] = reinterpret_cast<void*>(func);
	Syscalls::create_ec(cap,utcb,stacks[stack] + stack_top,cpunr,exbase,type,PD_ROOT);
	cur_cap++;
	stack++;
	utcb_addr -= PAGE_SIZE;
	return cap;
}

static void portal_startup(unsigned pid,void *tls,Utcb *utcb) {
	uint32_t *esp = reinterpret_cast<uint32_t*>(utcb->esp);
	thread_func func = *reinterpret_cast<thread_func*>(esp);
	Utcb *u = *reinterpret_cast<Utcb**>(esp + 2);
	func(u);
}

static void portal_map(unsigned pid,void *tls,Utcb *utcb) {
	utcb->set_header(0,utcb->head.untyped / 2);
	memmove(utcb->item_start(),utcb->msg,sizeof(unsigned) * utcb->head.typed * 2);
}

static void mythread(void *utcb) {
	char *cutcb = static_cast<char*>(utcb);
	while(1)
		;
}
