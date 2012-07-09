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
	Video::puts("Got timer irq ");
	Video::putn(counter++);
	Video::putc('\n');
	PIC::eoi(0x20);
}

static void keyboard() {
	Video::puts("Got keyboard irq: ");
	uint8_t sc;
	while((sc = Keyb::read())) {
		Video::puts("0x");
		Video::putu(sc,16);
		Video::putc(' ');
	}
	Video::putc('\n');
	PIC::eoi(0x21);
}

int main() {
	GDT::init();
	PIC::init();
	IDT::init();
	IDT::set(0x00,divbyzero);
	IDT::set(0x0D,gpf);
	IDT::set(0x20,timer);
	IDT::set(0x21,keyboard);
	PIT::init();
	Keyb::init();
	//Paging::init();
	//Paging::map(0x200000,0x400000,Paging::PRESENT);
	Video::clear();
	Video::puts("\n");

	Util::enable_ints();
	while(1)
		;
	return 0;
}
