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

struct Utcb
{
  enum {
    STACK_START = 512,          ///< Index where we store a "frame pointer" to the top of the stack
  };

  typedef struct Descriptor
  {
    unsigned short sel, ar;
    unsigned limit, base, res;
    void set(unsigned short _sel, unsigned _base, unsigned _limit, unsigned short _ar) { sel = _sel; base = _base; limit = _limit; ar = _ar; };
  } Descriptor;

  struct head {
    union {
      struct {
        unsigned short untyped;
        unsigned short typed;
      };
      unsigned mtr;
    };
    unsigned crd_translate;
    unsigned crd;
    unsigned nul_cpunr;
  } head;
  union {
    struct {
      unsigned     mtd;
      unsigned     inst_len, eip, efl;
      unsigned     intr_state, actv_state, inj_info, inj_error;
      union {
	struct {
#define GREG(NAME)					\
	  union {					\
	    struct {					\
	      unsigned char           NAME##l, NAME##h;	\
	    };						\
	    unsigned short          NAME##x;		\
	    unsigned           e##NAME##x;		\
	  }
#define GREG16(NAME)				\
	  union {				\
	    unsigned short          NAME;	\
	    unsigned           e##NAME;		\
	  }
	  GREG(a);    GREG(c);    GREG(d);    GREG(b);
	  GREG16(sp); GREG16(bp); GREG16(si); GREG16(di);
	};
	unsigned gpr[8];
      };
      unsigned long long qual[2];
      unsigned     ctrl[2];
      long long tsc_off;
      unsigned     cr0, cr2, cr3, cr4;
      unsigned     dr7, sysenter_cs, sysenter_esp, sysenter_eip;
      Descriptor   es, cs, ss, ds, fs, gs;
      Descriptor   ld, tr, gd, id;
    };
    unsigned msg[(4096 - sizeof(struct head)) / sizeof(unsigned)];
  };

  void set_header(unsigned untyped, unsigned typed) { head.untyped = untyped; head.typed = typed; }
  unsigned *item_start() { return reinterpret_cast<unsigned *>(this + 1) - 2*head.typed; }

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
