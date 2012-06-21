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

#include <kobj/Pt.h>
#include <kobj/Sm.h>
#include <subsystem/ChildManager.h>

using namespace nul;

static char prog[] = {
#include "../../test.dump"
};

int main() {
	// don't put it on the stack since its too large :)
	ChildManager *cm = new ChildManager();
	cm->load(reinterpret_cast<uintptr_t>(prog),sizeof(prog),"sub-test");
	cm->wait_for_all();
	return 0;
}
