/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <Types.h>

class ResourceSpace {
public:
	explicit ResourceSpace(cap_t portal,unsigned type)
		: _pt(portal), _type(type) {
	}

	void allocate(uintptr_t base,size_t size,unsigned rights,uintptr_t target = 0);

private:
	ResourceSpace(const ResourceSpace&);
	ResourceSpace& operator=(const ResourceSpace&);

private:
	cap_t _pt;
	unsigned _type;
};
