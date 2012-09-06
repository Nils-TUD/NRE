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

#include <ipc/Connection.h>
#include <services/Console.h>
#include <services/Storage.h>
#include <stream/ConsoleStream.h>

using namespace nre;

enum {
	SECTOR_SIZE		= 0x200
};

static void print_sector(OStream &os,Storage::sector_type sector,uintptr_t addr,size_t size) {
	uint *words = reinterpret_cast<uint*>(addr);
	for(size_t i = 0; i < size / sizeof(uint); ++i) {
		if((i % 4) == 0) {
			if(i > 0)
				os << "\n";
			os.writef("%016Lx: ",sector * size + i * sizeof(uint));
		}
		os.writef("%#08x ",words[i]);
	}
}

static void wait_for(StorageSession &sess,Storage::tag_type tag) {
	Storage::Packet *pk;
	Storage::tag_type rtag;
	do {
		pk = sess.consumer().get();
		rtag = pk->tag;
		sess.consumer().next();
	}
	while(rtag != tag);
}

int main() {
	Connection conscon("console");
	ConsoleSession cons(conscon,1);
	ConsoleStream s(cons,0);

	s << "\nWelcome to the interactive disk test utility!\n\n";
	size_t ctrl,disk;
	s << "Select the controller: ";
	s >> ctrl;
	s << "Select the disk: ";
	s >> disk;

	Storage::tag_type tag = 0;
	Connection storagecon("storage");
	DataSpace ds(0x1000,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW);
	StorageSession storage(storagecon,ds,ctrl,disk);

	Storage::Parameter params = storage.get_params();
	s << "\nConnected to disk '" << params.name << "' (";
	s << (params.sectors * params.sector_size) / (1024 * 1024) << " MiB in ";
	s << params.sectors << " sectors of " << params.sector_size << " bytes)\n";
	while(true) {
		Storage::sector_type sector;
		s << "Select the sector to read: ";
		s >> sector;
		try {
			storage.read(tag,sector,1,0);
			wait_for(storage,tag);
			s << "Contents:\n";
			print_sector(s,sector,ds.virt(),params.sector_size);
			s << "\n";
		}
		catch(const Exception &e) {
			s << e << "\n";
		}
		tag++;
	}
	return 0;
}
