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

static const Storage::sector_type cdsec	= 80;
static const size_t offset					= 0x200;
static Storage::tag_type tag				= 0;
static Storage::dma_type dma;

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

static void read_atapi(StorageSession &disk,Storage::Parameter &params,DataSpace &buffer) {
	clear_buffer(buffer);
	WVPRINTF("Reading sector %Lu",cdsec);
	dma.clear();
	dma.push(DMADesc(offset,params.sector_size));
	disk.read(tag,cdsec,dma);
	wait_for(disk,tag++);
	WVPASSEQ(reinterpret_cast<char*>(buffer.virt() + offset),CD_TEXT);
}

static void read_write_ata(StorageSession &disk,Storage::Parameter &params,DataSpace &buffer) {
	for(Storage::sector_type s = 0; s < params.sectors; ++s) {
		WVPRINTF("Writing to sector %Lu",s);
		prepare_buffer(buffer,offset,params.sector_size);
		dma.clear();
		dma.push(DMADesc(offset,params.sector_size));
		disk.write(tag,s,dma);
		wait_for(disk,tag);
		clear_buffer(buffer);
		WVPRINTF("Reading back from sector %Lu",s);
		dma.clear();
		dma.push(DMADesc(offset,params.sector_size));
		disk.read(tag,s,dma);
		wait_for(disk,tag++);
		check_buffer(buffer,offset,params.sector_size);
	}
}

static void read_invalid_sector(StorageSession &disk,Storage::Parameter &params,DataSpace &buffer) {
	clear_buffer(buffer);
	WVPRINTF("Reading invalid sector");
	try {
		dma.clear();
		dma.push(DMADesc(offset,params.sector_size));
		disk.read(tag,params.sectors + 1,dma);
		WVPASS(false);
	}
	catch(const Exception &e) {
		WVPRINTF("Got exception: %s",e.msg());
		WVPASS(true);
	}
}

static void runtest(Connection &storagecon,DataSpace &buffer,size_t d) {
	try {
		StorageSession disk(storagecon,buffer,d);
		Storage::Parameter params = disk.get_params();
		Serial::get() << "Connected to disk '" << params.name << "' (";
		Serial::get() << (params.sectors * params.sector_size) / (1024 * 1024) << " MiB in ";
		Serial::get() << params.sectors << " sectors of " << params.sector_size << " bytes)\n";

		read_invalid_sector(disk,params,buffer);

		if(params.flags & Storage::Parameter::FLAG_ATAPI)
			read_atapi(disk,params,buffer);
		else
			read_write_ata(disk,params,buffer);

		WVPRINTF("Testing flush cache");
		disk.flush(tag);
		wait_for(disk,tag++);
	}
	catch(const Exception &e) {
		Serial::get() << "Operation with " << d << " failed: " << e.msg() << "\n";
	}
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

	Connection storagecon("storage");
	DataSpace buffer(0x1000,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW);
	for(size_t d = 0; d < Storage::MAX_CONTROLLER * Storage::MAX_DRIVES; ++d)
		runtest(storagecon,buffer,d);
	return 0;
}
