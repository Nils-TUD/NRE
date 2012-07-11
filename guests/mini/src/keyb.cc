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

#include "keyb.h"
#include "video.h"

bool Keyb::wait_status(uint8_t mask,uint8_t value) {
	uint8_t status;
	do
		status = Ports::in<uint8_t>(PORT_CTRL);
	while((status & mask) != value);
	return (status & mask) == value;
}
bool Keyb::wait_ack() {
	uint8_t status;
	while(1) {
		status = Ports::in<uint8_t>(PORT_CTRL);
		if((status & STATUS_DATA_AVAIL) && Ports::in<uint8_t>(PORT_DATA) == ACK)
			return true;
	}
	return false;
}

bool Keyb::wait_output_full() {
	return wait_status(STATUS_DATA_AVAIL,STATUS_DATA_AVAIL);
}
bool Keyb::wait_input_empty() {
	return wait_status(STATUS_BUSY,0);
}

bool Keyb::read_cmd(uint8_t cmd,uint8_t &value) {
	if(!wait_input_empty())
		return false;
	Ports::out<uint8_t>(PORT_CTRL,cmd);
	if(!wait_output_full())
		return false;
	value = Ports::in<uint8_t>(PORT_DATA);
	return true;
}
bool Keyb::write_cmd(uint8_t cmd,uint8_t value) {
	if(!wait_input_empty())
		return false;
	Ports::out<uint8_t>(PORT_CTRL,cmd);
	if(!wait_input_empty())
		return false;
	Ports::out<uint8_t>(PORT_DATA,value);
	return true;
}

bool Keyb::enable_device() {
	if(!wait_input_empty())
		return false;
	Ports::out<uint8_t>(PORT_CTRL,KBC_CMD_ENABLE_KEYBOARD);
	if(!wait_input_empty())
		return false;
	return true;
}

void Keyb::init() {
	// clear keyboard buffer
	while(Ports::in<uint8_t>(PORT_CTRL) & STATUS_DATA_AVAIL)
		Ports::in<uint8_t>(PORT_DATA);

	uint8_t cmdbyte = 0;
	if(!read_cmd(KBC_CMD_READ_STATUS,cmdbyte))
		Video::puts("kb init failed\n");

	// enable irqs
	cmdbyte &= ~KBC_CMDBYTE_TRANSPSAUX;
	if(!write_cmd(KBC_CMD_SET_STATUS,cmdbyte | KBC_CMDBYTE_IRQ1 | KBC_CMDBYTE_IRQ2) ||
			!read_cmd(KBC_CMD_READ_STATUS,cmdbyte)) {
		Video::puts("kb init failed\n");
	}

	if(!enable_device())
		Video::puts("kb init failed\n");

	// read available scancodes
	while(read())
		;
}
