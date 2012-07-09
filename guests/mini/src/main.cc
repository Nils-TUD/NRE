#include "util.h"
#include "paging.h"
#include "video.h"
#include "idt.h"
#include "gdt.h"

extern "C" int main();
extern "C" void error();

void error() {
	Video::puts("Unexpected interrupt/exception\n");
}

int main() {
	GDT::init();
	//Paging::init();
	//Paging::map(0x200000,0x400000,Paging::PRESENT);
	//IDT::init();
	Video::clear();
	Video::puts("\n");
	for(int i = 0; ; i++) {
		Video::puts("Huhu!");
		Video::putc('0' + (i % 10));
		Video::putc('\n');
	}
	return 0;
}
