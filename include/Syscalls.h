/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#ifdef __i386__
#include <arch/x86/SyscallABI.h>
#else
#error "Unsupported architecture"
#endif

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
protected:
	unsigned _value;
	Desc(unsigned v) :
			_value(v) {
	}
public:
	unsigned value() {
		return _value;
	}
};

/**
 * A capability range descriptor;
 */
class Crd: public Desc {
public:
	unsigned order() {
		return ((_value >> 7) & 0x1f);
	}
	unsigned size() {
		return 1 << (order() + 12);
	}
	unsigned base() {
		return _value & ~0xfff;
	}
	unsigned attr() {
		return _value & 0x1f;
	}
	unsigned cap() {
		return _value >> 12;
	}
	explicit Crd(unsigned offset,unsigned order,unsigned attr) :
			Desc((offset << 12) | (order << 7) | attr) {
	}
	explicit Crd(unsigned v) :
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
public:
	enum ECType {
		EC_GLOBAL,
		EC_LOCAL
	};

private:
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
		SEMCTL,
		ASSIGN_PCI,
		ASSIGN_GSI,
		CREATE_ECCLIENT = CREATE_EC | FLAG0,
	};

public:
	static inline void call(cap_t pt) {
		SyscallABI::syscall(pt << 8 | IPC_CALL);
	}

	static inline void create_ec(cap_t ec,void *utcb,void *esp,cpu_t cpunr,unsigned excpt_base,
			ECType type,cap_t dstpd) {
		SyscallABI::syscall(ec << 8 | (type == EC_LOCAL ? CREATE_EC : CREATE_ECCLIENT),dstpd,
		        reinterpret_cast<SyscallABI::arg_t>(utcb) | cpunr,
		        reinterpret_cast<SyscallABI::arg_t>(esp),
		        excpt_base);
	}

	static inline void create_sc(cap_t sc,cap_t ec,Qpd qpd,cpu_t cpu,cap_t dstpd) {
		SyscallABI::syscall(sc << 8 | CREATE_SC,dstpd,ec,qpd.value(),cpu);
	}

	static inline void create_pt(cap_t pt,cap_t ec,uintptr_t eip,unsigned mtd,cap_t dstpd) {
		SyscallABI::syscall(pt << 8 | CREATE_PT,dstpd,ec,mtd,eip);
	}
};