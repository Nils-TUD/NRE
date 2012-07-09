/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

class PIT {
private:
	enum {
		FREQ = 1193182
	};

	enum {
		CTRL			= 0x43,
		CHAN0DIV		= 0x40
	};

	enum {
		CTRL_CHAN0		= 0x0,
		CTRL_CHAN1		= 0x40,
		CTRL_CHAN2		= 0x80
	};

	enum {
		CTRL_RWLO		= 0x10,
		CTRL_RWHI		= 0x20,
		CTRL_RWLOHI		= 0x30
	};

	enum {
		CTRL_MODE1		= 0x2,
		CTRL_MODE2		= 0x4,
		CTRL_MODE3		= 0x6,
		CTRL_MODE4		= 0x8,
		CTRL_MODE5		= 0xA,
	};

	enum {
		CTRL_CNTBIN16	= 0x0,
		CTRL_CNTBCD		= 0x1
	};

public:
	static void init();

private:
	PIT();
	~PIT();
	PIT(const PIT&);
	PIT& operator=(const PIT&);
};
