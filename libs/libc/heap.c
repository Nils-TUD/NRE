/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

typedef unsigned long size_t;

static char buffer[4096];
static size_t pos = 0;

void* malloc(size_t size) {
	void* res;
	if(pos + size >= sizeof(buffer))
		return 0;
	
	res = buffer + pos;
	pos += size;
	return res;
}

void free(void* p) {
	// do nothing for now
}
