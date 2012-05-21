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

#include <mem/DataSpace.h>

namespace nul {

class SharedDataSpace : public DataSpace {
public:
	SharedDataSpace(size_t size,Type type,Perm perm,uintptr_t addr = 0)
		: DataSpace(size,type,perm,addr), _sm(0) {
		UtcbFrame uf;
		uf.delegate(_sm.sel());
		uf << _addr << _size << _perm << _type;
		CPU::current().map_pt->call(uf);
	}
	virtual ~SharedDataSpace() {
	}

	virtual void map() {
		UtcbFrame uf;
		uf.translate(_sm.sel());
		uf << _addr << _size << _perm << _type;
		CPU::current().map_pt->call(uf);
		uf >> _addr;
	}
	virtual void unmap() {
		// TODO
	}

private:
	Sm _sm;
};

}
