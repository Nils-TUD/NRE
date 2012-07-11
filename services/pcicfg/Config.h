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

class Config {
public:
	explicit Config() {
	}
	virtual ~Config() {
	}

	virtual bool contains(uint32_t bdf,size_t offset) const = 0;
	virtual uintptr_t addr(uint32_t bdf,size_t offset) = 0;
	virtual uint32_t read(uint32_t bdf,size_t offset) = 0;
	virtual void write(uint32_t bdf,size_t offset,uint32_t value) = 0;
};
