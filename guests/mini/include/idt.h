#pragma once

#include "util.h"

class IDT {
private:
	struct Entry {
		/* The address[0..15] of the ISR */
		uint16_t offsetLow;
		/* Code selector that the ISR will use */
		uint16_t selector;
		/* these bits are fix: 0.1110.0000.0000b */
		uint16_t fix		: 13,
		/* the privilege level, 00 = ring0, 01 = ring1, 10 = ring2, 11 = ring3 */
		dpl			: 2,
		/* If Present is not set to 1, an exception will occur */
		present		: 1;
		/* The address[16..31] of the ISR */
		uint16_t	offsetHigh;
	} A_PACKED;
	
	struct Ptr {
		uint16_t size;
		uint32_t address;
	} A_PACKED;
	
	/* reserved by intel */
	enum {
		INTEL_RES1	= 2,
		INTEL_RES2	= 15
	};

public:
	typedef void (*callback_t)(void);

public:
	static void init();
	static void set(size_t irq,callback_t cb);

private:
	IDT();
	~IDT();
	IDT(const IDT&);
	IDT& operator=(const IDT&);

private:
	static Entry idt[];
};

