/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <arch/ExecEnv.h>
#include <kobj/Ports.h>
#include <mem/DataSpace.h>
#include <dev/Console.h>

#include "Screen.h"

class HostVGA : public Screen {
	enum Register {
		START_ADDR_HI	= 0xc,
		START_ADDR_LO	= 0xd
	};
	enum {
		PAGE_COUNT		= 8,
		VGA_MEM			= 0xb8000,
		VGA_MEM_SIZE	= nul::ExecEnv::PAGE_SIZE * PAGE_COUNT,
		COLS			= 80,
		ROWS			= 25,
	};

public:
	explicit HostVGA()
			: Screen(), _ports(0x3d4,2), _page(0),
			  _ds(VGA_MEM_SIZE,nul::DataSpaceDesc::LOCKED,nul::DataSpaceDesc::RW,VGA_MEM) {
	}

	virtual void paint(uint uid,uint8_t x,uint8_t y,uint8_t *buffer,size_t count);
	virtual void set_page(uint uid,uint page);

private:
	void write(Register reg,uint8_t val) {
		_ports.out<uint8_t>(reg,0);
		_ports.out<uint8_t>(val,1);
	}

	nul::Ports _ports;
	uint _page;
	nul::DataSpace _ds;
};
