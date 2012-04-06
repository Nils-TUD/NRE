/*
 * TODO comment me
 *
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NUL.
 *
 * NUL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NUL is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <utcb/UtcbHead.h>

class UtcbExc : public UtcbHead {
private:
	struct Descriptor {
		uint16_t sel,ar;
		uint32_t limit,base,res;
		void set(uint16_t _sel,uint32_t _base,uint32_t _limit,uint16_t _ar) {
			sel = _sel;
			base = _base;
			limit = _limit;
			ar = _ar;
		}
	};

public:
	uint32_t mtd;
	uint32_t inst_len,eip,efl;
	uint32_t intr_state,actv_state,inj_info,inj_error;
	union {
		struct {
			uint32_t eax,ecx,edx,ebx,esp,ebp,esi,edi;
		};
		uint32_t gpr[8];
	};
	uint64_t qual[2];
	uint32_t ctrl[2];
	int64_t tsc_off;
	uint32_t cr0,cr2,cr3,cr4;
	uint32_t dr7,sysenter_cs,sysenter_esp,sysenter_eip;
	Descriptor es,cs,ss,ds,fs,gs;
	Descriptor ld,tr,gd,id;
};
