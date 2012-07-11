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

#include <arch/Types.h>
#include <arch/ExecEnv.h>
#include <utcb/UtcbHead.h>
#include <cstring>

#define GREG(NAME)						\
	union {								\
		struct {						\
			uint8_t NAME##l, NAME##h;	\
		};								\
		uint16_t NAME##x;				\
		uint32_t e##NAME##x;			\
		word_t r##NAME##x;				\
	}

#define GREG16(NAME)					\
	union {								\
		uint16_t NAME;					\
		uint32_t e##NAME;				\
		word_t r##NAME;					\
	}

namespace nre {

class UtcbExc : public UtcbHead {
public:
	struct Descriptor {
		uint16_t sel,ar;
		uint32_t limit;
		union {
			uint64_t : 64;
			word_t base;
		};
		void set(uint16_t _sel,uint32_t _base,uint32_t _limit,uint16_t _ar) {
			sel = _sel;
			base = _base;
			limit = _limit;
			ar = _ar;
		}
	};

	union {
		struct {
			word_t mtd;
			word_t inst_len;
			GREG16(ip); GREG16(fl);
			uint32_t intr_state,actv_state,inj_info,inj_error;
			union {
				struct {
					GREG(a); GREG(c); GREG(d); GREG(b);
					GREG16(sp); GREG16(bp); GREG16(si); GREG16(di);
		#ifdef __x86_64__
					word_t r8,r9,r10,r11,r12,r13,r14,r15;
		#endif
				};
		#ifdef __x86_64__
				word_t gpr[16];
		#else
				word_t gpr[8];
		#endif
			};
			uint64_t qual[2];
			uint32_t ctrl[2];
			int64_t tsc_off;
			word_t cr0,cr2,cr3,cr4;
		#ifdef __x86_64__
			word_t cr8,reserved;
		#endif
			word_t dr7,sysenter_cs,sysenter_rsp,sysenter_rip;
			Descriptor es,cs,ss,ds,fs,gs;
			Descriptor ld,tr,gd,id;
		};
		word_t words[];
	};

	void clear() {
		memset(words,0,ExecEnv::PAGE_SIZE - sizeof(UtcbHead));
	}
};

}
