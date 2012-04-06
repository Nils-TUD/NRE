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

class Utcb {
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

	struct head {
		union {
			struct {
				uint16_t untyped;
				uint16_t typed;
			};
			uint32_t mtr;
		};
		uint32_t crd_translate;
		uint32_t crd;
		uint32_t nul_cpunr;
	} head;

	union {
		struct {
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
		uint32_t msg[(4096 - sizeof(struct head)) / sizeof(uint32_t)];
	};

	void set_header(unsigned untyped,unsigned typed) {
		head.untyped = untyped;
		head.typed = typed;
	}
	unsigned *item_start() {
		return reinterpret_cast<unsigned *>(this + 1) - 2 * head.typed;
	}

	/**
	 * If you mixing code which manipulates the utcb by its own and you use this Utcb/Frame code,
	 * you have to fix up your utcb after the code manipulated the utcb by its own. Otherwise some of the
	 * assertion in the Frame code will trigger because the Utcb/Frame code assumes it's the only
	 * one who manipulates the utcb. In general avoid this mixing, however in sigma0 it's not done everywhere.
	 */
	void reset() {
		head.mtr = 0;
		this->msg[STACK_START] = 0;
	}
};
