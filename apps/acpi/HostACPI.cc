/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <mem/DataSpace.h>
#include <stream/Serial.h>
#include <cstring>

#include "HostACPI.h"

using namespace nul;

HostACPI::HostACPI() : _count(), _tables(), _rsdp(get_rsdp()) {
	// get rsdt
	DataSpace *ds;
	ds = new DataSpace(ExecEnv::PAGE_SIZE,DataSpaceDesc::LOCKED,DataSpaceDesc::R,_rsdp->rsdtAddr);
	RSDT *rsdt = reinterpret_cast<RSDT*>(ds->virt() + (_rsdp->rsdtAddr & (ExecEnv::PAGE_SIZE - 1)));
	if(rsdt->length > ExecEnv::PAGE_SIZE) {
		uintptr_t size = rsdt->length;
		delete ds;
		ds = new DataSpace(size,DataSpaceDesc::LOCKED,DataSpaceDesc::R,_rsdp->rsdtAddr);
		rsdt = reinterpret_cast<RSDT*>(ds->virt() + (_rsdp->rsdtAddr & (ExecEnv::PAGE_SIZE - 1)));
	}

	if(checksum(reinterpret_cast<char*>(rsdt),rsdt->length) != 0)
		throw Exception(E_NOT_FOUND,"RSDT checksum invalid");

	// find out the address range of all tables to map them contiguously
	uint32_t *tables = reinterpret_cast<uint32_t*>(rsdt + 1);
	uintptr_t min = 0xFFFFFFFF, max = 0;
	size_t count = (rsdt->length - sizeof(RSDT)) / 4;
	for(size_t i = 0; i < count; i++) {
		DataSpace *tmp = new DataSpace(ExecEnv::PAGE_SIZE,DataSpaceDesc::LOCKED,DataSpaceDesc::R,tables[i]);
		RSDT *tbl = reinterpret_cast<RSDT*>(tmp->virt() + (tables[i] & (ExecEnv::PAGE_SIZE - 1)));
		/* determine the address range for the RSDT's */
		if(tables[i] < min)
			min = tables[i];
		if(tables[i] + tbl->length > max)
			max = tables[i] + tbl->length;
		delete tmp;
	}

	// map them and put all pointers in an array
	size_t size = (max + ExecEnv::PAGE_SIZE * 2 - 1) - (min & ~(ExecEnv::PAGE_SIZE - 1));
	_ds = new DataSpace(size,DataSpaceDesc::LOCKED,DataSpaceDesc::R,min);
	_count = count;
	_tables = new RSDT*[count];
	for(size_t i = 0; i < count; i++) {
		_tables[i] = reinterpret_cast<RSDT*>(
				_ds->virt() + (min & (ExecEnv::PAGE_SIZE - 1)) - min + tables[i]);
	}
}

uintptr_t HostACPI::find(const char *name,uint instance) {
	for(size_t i = 0; i < _count; i++) {
		if(memcmp(_tables[i]->signature,name,4) == 0 && instance-- == 0) {
			if(checksum(reinterpret_cast<char*>(_tables[i]),_tables[i]->length) != 0)
				throw Exception(E_NOT_FOUND);
			return reinterpret_cast<uintptr_t>(_tables[i]) - _ds->virt();
		}
	}
	return 0;
}

HostACPI::RSDP *HostACPI::get_rsdp() {
	// search in BIOS readonly memory range
	DataSpace *ds = new DataSpace(BIOS_MEM_SIZE,DataSpaceDesc::LOCKED,DataSpaceDesc::R,BIOS_MEM_ADDR);
	char *area = reinterpret_cast<char*>(ds->virt());
	for(uintptr_t off = 0; off < BIOS_MEM_SIZE; off += 16) {
		if(memcmp(area + off,"RSD PTR ",8) == 0 && checksum(area + off,20) == 0)
			return reinterpret_cast<RSDP*>(area + off);
	}
	delete ds;

	// search in ebda
	DataSpace bios(BIOS_SIZE,DataSpaceDesc::LOCKED,DataSpaceDesc::R,BIOS_ADDR);
	uint16_t ebda = *reinterpret_cast<uint16_t*>(bios.virt() + BIOS_EBDA_OFF);
	ds = new DataSpace(BIOS_EBDA_SIZE,DataSpaceDesc::LOCKED,DataSpaceDesc::R,ebda * 16);
	area = reinterpret_cast<char*>(ds->virt());
	for(uintptr_t off = 0; off < BIOS_EBDA_SIZE; off += 16) {
		if(memcmp(area + off,"RSD PTR ",8) == 0 && checksum(area + off,20) == 0)
			return reinterpret_cast<RSDP*>(area + off);
	}
	delete ds;

	throw Exception(E_NOT_FOUND,"Unable to find RSDP");
}
