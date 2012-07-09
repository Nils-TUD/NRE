/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include "pic.h"
#include "ports.h"
#include "util.h"

void PIC::init() {
	/* starts the initialization. we want to send a ICW4 */
	Ports::out<uint8_t>(MASTER_CMD,ICW1_INIT | ICW1_NEED_ICW4);
	Ports::out<uint8_t>(SLAVE_CMD,ICW1_INIT | ICW1_NEED_ICW4);
	/* remap the irqs to 0x20 and 0x28 */
	Ports::out<uint8_t>(MASTER_DATA,REMAP_MASTER);
	Ports::out<uint8_t>(SLAVE_DATA,REMAP_SLAVE);
	/* continue */
	Ports::out<uint8_t>(MASTER_DATA,4);
	Ports::out<uint8_t>(SLAVE_DATA,2);

	/* we want to use 8086 mode */
	Ports::out<uint8_t>(MASTER_DATA,ICW4_8086);
	Ports::out<uint8_t>(SLAVE_DATA,ICW4_8086);

	/* enable all interrupts (set masks to 0) */
	Ports::out<uint8_t>(MASTER_DATA,0x00);
	Ports::out<uint8_t>(SLAVE_DATA,0x00);
}

void PIC::eoi(int irq) {
	/* do we have to send EOI? */
	if(irq >= REMAP_MASTER && irq <= REMAP_MASTER + 16) {
	    if(irq >= REMAP_SLAVE) {
	    	/* notify the slave */
	    	Ports::out<uint8_t>(SLAVE_CMD,EOI);
	    }

	    /* notify the master */
	    Ports::out<uint8_t>(MASTER_CMD,EOI);
    }
}
