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
#include <Test.h>

using namespace nre;

#define CD_TEXT		\
	"That is a test!!\nMore testing\n"

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

static void check_buffer(const DataSpace &buffer,size_t offset,size_t size) {
	uint8_t *bytes = reinterpret_cast<uint8_t*>(buffer.virt() + offset);
	uint expected = 0;
	uint received = 0;
	for(size_t i = 0; i < size; ++i) {
		expected += i & 0xFF;
		received += bytes[i];
	}
	WVPASSEQ(expected,received);
}

static void clear_buffer(const DataSpace &buffer) {
	memset(reinterpret_cast<void*>(buffer.virt()),0,buffer.size());
}

static void prepare_buffer(const DataSpace &buffer,size_t offset,size_t size) {
	uint8_t *bytes = reinterpret_cast<uint8_t*>(buffer.virt() + offset);
	for(size_t i = 0; i < size; ++i)
		bytes[i] = i & 0xFF;
}

int main() {
	Connection conscon("console");
	ConsoleSession cons(conscon,1,String("DiskTest"));
	ConsoleStream s(cons,0);
	s << "Welcome to the disk test program!\n\n";
	s << "WARNING: This test will write on every sector of all harddisks!!!\n";
	s << "ARE YOU SURE YOU WANT TO DO THAT (enter '4711' for yes): ";
	uint answer;
	s >> answer;
	if(answer != 4711) {
		s << "Ok, bye!\n";
		return 0;
	}

	const Storage::sector_type cdsec = 80;
	const size_t offset = 0x200;
	Storage::tag_type tag = 0;
	Connection storagecon("storage");
	DataSpace buffer(0x1000,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW);
	for(size_t d = 0; d < Storage::MAX_CONTROLLER * Storage::MAX_DRIVES; ++d) {
		try {
			StorageSession disk(storagecon,buffer,d);
			Storage::Parameter params = disk.get_params();
			Serial::get() << "Connected to disk '" << params.name << "' (";
			Serial::get() << (params.sectors * params.sector_size) / (1024 * 1024) << " MiB in ";
			Serial::get() << params.sectors << " sectors of " << params.sector_size << " bytes)\n";

			clear_buffer(buffer);
			WVPRINTF("Reading invalid sector");
			try {
				disk.read(tag,params.sectors + 1,1,offset);
				WVPASS(false);
			}
			catch(const Exception &e) {
				WVPRINTF("Got exception: %s",e.msg());
				WVPASS(true);
			}

			if(params.flags & Storage::Parameter::FLAG_ATAPI) {
				clear_buffer(buffer);
				WVPRINTF("Reading sector %Lu",cdsec);
				disk.read(tag,cdsec,1,offset);
				wait_for(disk,tag++);
				WVPASSEQ(reinterpret_cast<char*>(buffer.virt() + offset),CD_TEXT);
			}
			else {
				for(Storage::sector_type s = 0; s < params.sectors; ++s) {
					WVPRINTF("Writing to sector %Lu",s);
					prepare_buffer(buffer,offset,params.sector_size);
					disk.write(tag,s,1,offset);
					wait_for(disk,tag);
					clear_buffer(buffer);
					WVPRINTF("Reading back from sector %Lu",s);
					disk.read(tag,s,1,offset);
					wait_for(disk,tag++);
					check_buffer(buffer,offset,params.sector_size);
				}
			}

			WVPRINTF("Testing flush cache");
			disk.flush(tag);
			wait_for(disk,tag++);
		}
		catch(const Exception &e) {
			Serial::get() << "Operation with " << d << " failed: " << e.msg() << "\n";
		}
	}
	return 0;
}
