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
#include <mem/DataSpace.h>
#include <dev/Console.h>

class HostVGA {
	enum {
		VGA_MEM			= 0xb8000,
		VGA_MEM_SIZE	= nul::ExecEnv::PAGE_SIZE * 8,
		COLS			= 80,
		ROWS			= 25,
	};

public:
	explicit HostVGA() : _page(1), _ds(VGA_MEM_SIZE,nul::DataSpaceDesc::ANONYMOUS,nul::DataSpaceDesc::RW,VGA_MEM) {
	}

	void put(const nul::Console::Packet &pk);

private:
	int _page;
	nul::DataSpace _ds;
};
