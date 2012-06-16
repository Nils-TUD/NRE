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
#include <Desc.h>

namespace nul {

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
		EC_CTL,
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
	enum EcOp {
		RECALL	= 0
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

	static inline void create_pt(capsel_t pt,capsel_t ec,uintptr_t eip,Mtd mtd,capsel_t dstpd) {
		SyscallABI::syscall(pt << 8 | CREATE_PT,dstpd,ec,mtd.value(),eip);
	}

	static inline void create_pd(capsel_t pd,Crd pt_crd,capsel_t dstpd) {
		SyscallABI::syscall(pd << 8 | CREATE_PD,dstpd,pt_crd.value());
	}

	static inline void create_sm(capsel_t sm,unsigned initial,capsel_t dstpd) {
		SyscallABI::syscall(sm << 8 | CREATE_SM,dstpd,initial);
	}

	static inline void ec_ctrl(capsel_t ec,EcOp op) {
		SyscallABI::syscall(ec << 8 | EC_CTL | op);
	}

	static inline void sm_ctrl(capsel_t sm,SmOp op) {
		SyscallABI::syscall(sm << 8 | SM_CTL | op);
	}

	static inline void assign_gsi(capsel_t sm,cpu_t cpu,void *pci_cfg_mem = 0,uint64_t *msi_address = 0,word_t *msi_value = 0) {
		word_t out1,out2;
		SyscallABI::syscall(sm << 8 | ASSIGN_GSI,reinterpret_cast<word_t>(pci_cfg_mem),cpu,0,0,out1,out2);
		if(msi_address)
			*msi_address = out1;
		if(msi_value)
			*msi_value = out2;
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
