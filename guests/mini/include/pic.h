/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

class PIC {
private:
	enum {
		MASTER			= 0x20,
		SLAVE			= 0xA0,
	};

	enum {
		MASTER_CMD		= MASTER,
		MASTER_DATA		= MASTER + 1,
		SLAVE_CMD		= SLAVE,
		SLAVE_DATA		= SLAVE + 1
	};

	enum {
		EOI				= 0x20
	};

	enum {
		ICW1_NEED_ICW4	= 0x01,
		ICW1_SINGLE		= 0x02,
		ICW1_INTERVAL4	= 0x04,
		ICW1_LEVEL		= 0x08,
		ICW1_INIT		= 0x10
	};

	enum {
		ICW4_8086		= 0x01,
		ICW4_AUTO		= 0x02,
		ICW4_BUF_SLAVE	= 0x08,
		ICW4_BUF_MASTER	= 0x0C,
		ICW4_SFNM		= 0x10
	};

	enum {
		REMAP_MASTER	= 0x20,
		REMAP_SLAVE		= 0x28
	};

public:
	static void init();
	static void eoi(int irq);

private:
	PIC();
	~PIC();
	PIC(const PIC&);
	PIC& operator=(const PIC&);
};
