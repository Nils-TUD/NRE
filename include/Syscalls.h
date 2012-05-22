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

#include <arch/SyscallABI.h>

namespace nul {

enum {
	MTD_GPR_ACDB = 1ul << 0,
	MTD_GPR_BSD = 1ul << 1,
	MTD_RSP = 1ul << 2,
	MTD_RIP_LEN = 1ul << 3,
	MTD_RFLAGS = 1ul << 4,
	MTD_DS_ES = 1ul << 5,
	MTD_FS_GS = 1ul << 6,
	MTD_CS_SS = 1ul << 7,
	MTD_TR = 1ul << 8,
	MTD_LDTR = 1ul << 9,
	MTD_GDTR = 1ul << 10,
	MTD_IDTR = 1ul << 11,
	MTD_CR = 1ul << 12,
	MTD_DR = 1ul << 13,
	MTD_SYSENTER = 1ul << 14,
	MTD_QUAL = 1ul << 15,
	MTD_CTRL = 1ul << 16,
	MTD_INJ = 1ul << 17,
	MTD_STATE = 1ul << 18,
	MTD_TSC = 1ul << 19,
	MTD_IRQ = MTD_RFLAGS | MTD_STATE | MTD_INJ | MTD_TSC,
	MTD_ALL = (~0U >> 12) & ~MTD_CTRL
};

enum {
	DESC_TYPE_MEM = 1,
	DESC_TYPE_IO = 2,
	DESC_TYPE_CAP = 3,
	DESC_RIGHT_R = 0x4,
	DESC_RIGHT_EC_RECALL = 0x4,
	DESC_RIGHT_PD = 0x4,
	DESC_RIGHT_EC = 0x8,
	DESC_RIGHT_SC = 0x10,
	DESC_RIGHT_PT = 0x20,
	DESC_RIGHT_SM = 0x40,
	DESC_RIGHTS_ALL = 0x7c,
	DESC_MEM_ALL = DESC_TYPE_MEM | DESC_RIGHTS_ALL,
	DESC_IO_ALL = DESC_TYPE_IO | DESC_RIGHTS_ALL,
	DESC_CAP_ALL = DESC_TYPE_CAP | DESC_RIGHTS_ALL,
	MAP_HBIT = 0x801,
	MAP_EPT = 0x401,
	MAP_DPT = 0x201,
	MAP_MAP = 1,	///< Delegate typed item
};

class Desc {
	word_t _value;
protected:
	Desc(word_t v) : _value(v) {
	}
public:
	word_t value() const {
		return _value;
	}
};

/**
 * A capability range descriptor;
 */
class Crd: public Desc {
public:
	unsigned order() const {
		return ((value() >> 7) & 0x1f);
	}
	word_t size() const {
		return 1 << (order() + 12);
	}
	word_t base() const {
		return value() & ~0xfff;
	}
	unsigned attr() const {
		return value() & 0x1f;
	}
	word_t cap() const {
		return value() >> 12;
	}
	explicit Crd(word_t offset,unsigned order,unsigned attr) :
			Desc((offset << 12) | (order << 7) | attr) {
	}
	explicit Crd(word_t v) :
			Desc(v) {
	}
};

/**
 * A quantum+period descriptor.
 */
class Qpd: public Desc {
	enum { DEFAULT_QUANTUM = 10000, DEFAULT_PRIORITY = 1 };
public:
	Qpd(unsigned prio = DEFAULT_PRIORITY,unsigned quantum = DEFAULT_QUANTUM) :
			Desc((quantum << 12) | prio) {
	}
};

class Syscalls {
	enum {
		FLAG0	= 1 << 4,
		FLAG1	= 1 << 5,
	};
	enum {
		IPC_CALL,
		IPC_REPLY,
		CREATE_PD,
		CREATE_EC,
		CREATE_SC,
		CREATE_PT,
		CREATE_SM,
		REVOKE,
		LOOKUP,
		RECALL,
		SC_CTL,
		SM_CTL,
		ASSIGN_PCI,
		ASSIGN_GSI,
		CREATE_EC_GLOBAL = CREATE_EC | FLAG0,
		REVOKE_MYSELF  = REVOKE | FLAG0,
	};

public:
	enum ECType {
		EC_GLOBAL,
		EC_LOCAL
	};
	enum SmOp {
		SM_UP	= 0,
		SM_DOWN = FLAG0,
		SM_ZERO	= FLAG0 | FLAG1
	};

	static inline void call(capsel_t pt) {
		SyscallABI::syscall(pt << 8 | IPC_CALL);
	}

	static inline void create_ec(capsel_t ec,void *utcb,void *esp,cpu_t cpunr,unsigned excpt_base,
			ECType type,capsel_t dstpd) {
		SyscallABI::syscall(ec << 8 | (type == EC_LOCAL ? CREATE_EC : CREATE_EC_GLOBAL),dstpd,
		        reinterpret_cast<word_t>(utcb) | cpunr,
		        reinterpret_cast<word_t>(esp),
		        excpt_base);
	}

	static inline void create_sc(capsel_t sc,capsel_t ec,Qpd qpd,capsel_t dstpd) {
		SyscallABI::syscall(sc << 8 | CREATE_SC,dstpd,ec,qpd.value());
	}

	static inline void create_pt(capsel_t pt,capsel_t ec,uintptr_t eip,unsigned mtd,capsel_t dstpd) {
		SyscallABI::syscall(pt << 8 | CREATE_PT,dstpd,ec,mtd,eip);
	}

	static inline void create_pd(capsel_t pd,Crd pt_crd,unsigned dstpd) {
		SyscallABI::syscall(pd << 8 | CREATE_PD,dstpd,pt_crd.value());
	}

	static inline void create_sm(capsel_t sm,unsigned initial,capsel_t dstpd) {
		SyscallABI::syscall(sm << 8 | CREATE_SM,dstpd,initial);
	}

	static inline void sm_ctrl(capsel_t sm,SmOp op) {
		SyscallABI::syscall(sm << 8 | SM_CTL | op);
	}

	static inline void revoke(Crd crd,bool myself) {
		SyscallABI::syscall(myself ? REVOKE_MYSELF : REVOKE,crd.value());
	}

private:
	Syscalls();
	~Syscalls();
	Syscalls(const Syscalls&);
	Syscalls& operator=(const Syscalls&);
};

}
