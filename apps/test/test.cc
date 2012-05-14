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

#include <kobj/LocalEc.h>
#include <kobj/GlobalEc.h>
#include <kobj/Sc.h>
#include <kobj/Sm.h>
#include <service/Client.h>
#include <stream/OStringStream.h>

using namespace nul;

static void write(void *) {
	char buf[128];
	Client scr("screen");
	for(uint i = 0; ; ++i) {
		UtcbFrame uf;
		OStringStream::format(buf,sizeof(buf),"Example text %u",i);
		uf << String(buf);
		scr.pt()->call(uf);
	}
}

int main() {
	const Hip& hip = Hip::get();
	for(Hip::cpu_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled())
			new Sc(new GlobalEc(write,0,it->id()),Qpd());
	}

	Sm sm(0);
	sm.down();
	return 0;
}
