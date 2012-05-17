/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <cap/CapRange.h>
#include <utcb/UtcbFrame.h>
#include <CPU.h>

namespace nul {

class Caps {
public:
	enum {
		TYPE_MEM	= 1,
		TYPE_IO		= 2,
		TYPE_CAP	= 3
	};

	enum {
		// memory capabilities
		MEM_R		= 1 << 2,
		MEM_W		= 1 << 3,
		MEM_X		= 1 << 4,
		MEM_RW		= MEM_R | MEM_W,
		MEM_RX		= MEM_R | MEM_X,
		MEM_RWX		= MEM_R | MEM_W | MEM_X,

		// I/O capabilities
		IO_A		= 1 << 2,

		// object capabilities for Pd
		OBJ_PD_PD	= 1 << 2,		// create_pd
        OBJ_PD_EC	= 1 << 3,		// create_ec
        OBJ_PD_SC	= 1 << 4,		// create_sc
        OBJ_PD_PT	= 1 << 5,		// create_pt
        OBJ_PD_SM	= 1 << 6,		// create_sm

		// object capabilities for Ec
        OBJ_EC_CT	= 1 << 2,		// ec_ctrl
        OBJ_EC_SC	= 1 << 4,		// create_sc
        OBJ_EC_PT	= 1 << 5,		// create_pt

        // object capabilities for Sc
        OBJ_SC_CT	= 1 << 2,		// sc_ctrl

        // object capabilities for Sm
        OBJ_SM_UP	= 1 << 2,		// sm_ctrl[up]
        OBJ_SM_DN	= 1 << 3,		// sm_ctrl[down]

        ALL			= 0x1F << 2
	};

	static void allocate(const CapRange& caps) {
		UtcbFrame uf;
		uf.set_receive_crd(Crd(0,31,caps.attr()));
		uf << caps;
		CPU::current().map_pt->call(uf);
	}

	static void free(const CapRange&) {
		// TODO
	}

private:
	Caps();
	~Caps();
	Caps(const Caps&);
	Caps& operator=(const Caps&);
};

}
