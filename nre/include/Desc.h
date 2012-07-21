/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <arch/Types.h>
#include <arch/Defines.h>
#include <stream/OStream.h>

namespace nre {

/**
 * Base class for all decriptors. Note that we don't support polymorphism here.
 */
class Desc {
	word_t _value;
protected:
	explicit Desc(word_t v) : _value(v) {
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
	enum {
		MEM		= 1,
		IO		= 2,
		OBJ		= 3,
	};
	enum {
		// memory capabilities
		R		= 1 << 2,
		W		= 1 << 3,
		X		= 1 << 4,
		RW		= R | W,
		RX		= R | X,
		RWX		= R | W | X,

		// I/O capabilities
		A		= 1 << 2,

		// object capabilities for Pd
		PD_PD	= 1 << 2,		// create_pd
		PD_EC	= 1 << 3,		// create_ec
		PD_SC	= 1 << 4,		// create_sc
		PD_PT	= 1 << 5,		// create_pt
		PD_SM	= 1 << 6,		// create_sm

		// object capabilities for Ec
		EC_CT	= 1 << 2,		// ec_ctrl
		EC_SC	= 1 << 4,		// create_sc
		EC_PT	= 1 << 5,		// create_pt

		// object capabilities for Sc
		SC_CT	= 1 << 2,		// sc_ctrl

		// object capabilities for Sm
		SM_UP	= 1 << 2,		// sm_ctrl[up]
		SM_DN	= 1 << 3,		// sm_ctrl[down]

		// abbreviations
		MEM_ALL		= MEM | RWX,
		IO_ALL		= IO | A,
		OBJ_ALL		= OBJ | (0x1F << 2)
	};

	bool is_null() const {
		return value() == 0;
	}
	word_t offset() const {
		return value() >> 12;
	}
	uint order() const {
		return ((value() >> 7) & 0x1f);
	}
	uint attr() const {
		return value() & 0x1f;
	}

	explicit Crd(word_t offset,uint order,uint attr) :
			Desc((offset << 12) | (order << 7) | attr) {
	}
	explicit Crd(word_t v) :
			Desc(v) {
	}
};

static inline OStream &operator<<(OStream &os,const Crd &crd) {
	static const char *types[] = {"NULL","MEM","IO","OBJ"};
	os.writef("Crd[type=%s offset=%#x order=%#x attr=%#x]",types[crd.attr() & 0x3],
			crd.offset(),crd.order(),crd.attr());
	return os;
}

/**
 * A message-transfer descriptor
 */
class Mtd : public Desc {
public:
	enum {
		GPR_ACDB	= 1ul << 0,
		GPR_BSD		= 1ul << 1,
		RSP			= 1ul << 2,
		RIP_LEN		= 1ul << 3,
		RFLAGS		= 1ul << 4,
		DS_ES		= 1ul << 5,
		FS_GS		= 1ul << 6,
		CS_SS		= 1ul << 7,
		TR			= 1ul << 8,
		LDTR		= 1ul << 9,
		GDTR		= 1ul << 10,
		IDTR		= 1ul << 11,
		CR			= 1ul << 12,
		DR			= 1ul << 13,
		SYSENTER	= 1ul << 14,
		QUAL		= 1ul << 15,
		CTRL		= 1ul << 16,
		INJ			= 1ul << 17,
		STATE		= 1ul << 18,
		TSC			= 1ul << 19,
		IRQ			= RFLAGS | STATE | INJ | TSC,
		ALL			= (~0U >> 12) & ~CTRL
	};

	explicit Mtd(word_t flags = 0) : Desc(flags) {
	}
};

static inline OStream &operator<<(OStream &os,const Mtd &mtd) {
	static const char *flags[] = {
		"GPR_ACDB","GPR_BSD","RSP","RIP_LEN","RFLAGS","DS_ES","FS_GS","CS_SS","TR","LDTR","GDTR",
		"IDTR","CR","DR","SYSENTER","QUAL","CTRL","INJ","STATE","TSC"
	};
	os << "Mtd[";
	bool first = true;
	for(size_t i = 0; i < ARRAY_SIZE(flags); ++i) {
		if(mtd.value() & (1 << i)) {
			if(!first)
				os << " ";
			os << flags[i];
			first = false;
		}
	}
	os << "]";
	return os;
}

/**
 * A quantum+period descriptor.
 */
class Qpd: public Desc {
	enum {
		DEFAULT_QUANTUM = 10000,
		DEFAULT_PRIORITY = 1
	};

public:
	explicit Qpd(uint prio = DEFAULT_PRIORITY,uint quantum = DEFAULT_QUANTUM) :
			Desc((quantum << 12) | prio) {
	}

	uint prio() const {
		return value() & 0xFFF;
	}
	uint quantum() const {
		return value() >> 12;
	}
};

static inline OStream &operator<<(OStream &os,const Qpd &qpd) {
	os << "Qpd[prio=" << qpd.prio() << " quantum=" << qpd.quantum() << "]";
	return os;
}

}
