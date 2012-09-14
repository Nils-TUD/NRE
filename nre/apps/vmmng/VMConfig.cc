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

#include "VMConfig.h"

using namespace nre;

void VMConfig::start(ChildManager *cm) {
	const char *lines = reinterpret_cast<const char*>(_ds.virt());
	const char *end = lines + strlen(lines);
	for(size_t i = 0; lines < end; ++i) {
		size_t namelen;
		const char *name = get_name(lines,namelen);
		Hip::mem_iterator mod = get_module(name,namelen);
		DataSpace ds(mod->size,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::R,mod->addr);
		ChildConfig cfg(i);
		cm->load(ds.virt(),mod->size,get_cmdline(lines),cfg);
	}
}

const char *VMConfig::get_cmdline(const char *begin) {
	static char buffer[256];
	const char *end = strchr(begin,'\n');
	if(!end)
		end = begin + strlen(begin);
	memcpy(buffer,begin,end - begin + 1);
	return buffer;
}

Hip::mem_iterator VMConfig::get_module(const char *name,size_t namelen) {
	const Hip &hip = Hip::get();
	for(Hip::mem_iterator mem = hip.mem_begin(); mem != hip.mem_end(); ++mem) {
		if(strncmp(mem->cmdline(),name,namelen) == 0)
			return mem;
	}
	throw Exception(E_NOT_FOUND,64,"Unable to find module '%.*s'",name,namelen);
}

const char *VMConfig::get_name(const char *str,size_t &len) {
	const char *space = strchr(str,' ');
	if(!space)
		space = str + strlen(str);
	len = space - str;
	return str;
}
