#include "util.h"
#include "paging.h"
#include "video.h"
#include "idt.h"
#include "gdt.h"
#include "ports.h"
#include "pit.h"
#include "pic.h"
#include "keyb.h"

extern "C" int main();

static int counter = 0;

static void divbyzero() {
	Video::puts("Divide by zero\n");
}

static void gpf() {
	Video::puts("General protection fault\n");
}

static void timer() {
	Video::printf("Got timer irq %d\n",counter++);
	PIC::eoi(0x20);
}

static void keyboard() {
	Video::puts("Got keyboard irq: ");
	uint8_t sc;
	while((sc = Keyb::read()))
		Video::printf("0x%x ",sc);
	Video::putc('\n');
	PIC::eoi(0x21);
}

int main() {
	GDT::init();
	Paging::init();
	PIC::init();
	IDT::init();
	IDT::set(0x00,divbyzero);
	IDT::set(0x0D,gpf);
	IDT::set(0x20,timer);
	IDT::set(0x21,keyboard);
	PIT::init();
	Keyb::init();
	Video::clear();
	Video::puts("\n");

	Paging::map(0x200000,0x400000,Paging::PRESENT | Paging::WRITABLE);
	int *addr = reinterpret_cast<int*>(0x200000);
	*addr = 4;

	Util::enable_ints();
	while(1)
		;
	return 0;
}
