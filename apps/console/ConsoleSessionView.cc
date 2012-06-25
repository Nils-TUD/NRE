/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <RCU.h>
#include <cstring>

#include "ConsoleSessionView.h"
#include "ConsoleSessionData.h"

using namespace nul;

ForwardCycler<CPU::iterator,LockPolicyDefault<SpinLock> > ConsoleSessionView::_cpus(CPU::begin(),CPU::end());

ConsoleSessionView::ConsoleSessionView(ConsoleSessionData *sess,uint id,DataSpace *in_ds,DataSpace *out_ds)
	: RCUObject(), DListItem(), _id(id), _ec(receiver,_cpus.next()->id), _sc(&_ec,Qpd()),
	  _in_ds(in_ds), _out_ds(out_ds),
	  _buffer(Console::VIEW_COUNT * ExecEnv::PAGE_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW),
	  _prod(in_ds,false,false), _cons(out_ds,false), _sess(sess) {
	_ec.set_tls<ConsoleSessionView*>(Ec::TLS_PARAM,this);
	memset(reinterpret_cast<void*>(_buffer.virt()),0,_buffer.size());
	_sc.start();
}

ConsoleSessionView::~ConsoleSessionView() {
	delete _out_ds;
	delete _in_ds;
}

void ConsoleSessionView::invalidate() {
	_cons.stop();
}

bool ConsoleSessionView::is_active() const {
	return _sess->is_active(this);
}

void ConsoleSessionView::put(const Console::SendPacket &pk) {
	if(pk.y < Screen::ROWS && pk.x < Screen::COLS) {
		uint8_t *buf = reinterpret_cast<uint8_t*>(_buffer.virt()) + _id * ExecEnv::PAGE_SIZE;
		uint8_t *pos = buf + pk.y * Screen::COLS * 2 + pk.x * 2;
		*pos = pk.character;
		*(pos + 1) = pk.color;
	}
}

void ConsoleSessionView::scroll() {
	char *screen = reinterpret_cast<char*>(_buffer.virt()) + _id * ExecEnv::PAGE_SIZE;
	memmove(screen,screen + Screen::COLS * 2,Screen::COLS * (Screen::ROWS - 1) * 2);
	memset(screen + Screen::COLS * (Screen::ROWS - 1) * 2,0,Screen::COLS * 2);
}

void ConsoleSessionView::repaint() {
	ConsoleService *s = ConsoleService::get();
	uint8_t *buf = reinterpret_cast<uint8_t*>(_buffer.virt()) + _id * ExecEnv::PAGE_SIZE;
	for(uint8_t y = 0; y < Screen::ROWS; ++y) {
		for(uint8_t x = 0; x < Screen::COLS; ++x) {
			uint8_t *pos = buf + y * Screen::COLS * 2 + x * 2;
			s->screen().paint(x,y,*pos,*(pos + 1));
		}
	}
}

void ConsoleSessionView::receiver(void *) {
	ScopedLock<RCULock> guard(&RCU::lock());
	ConsoleService *s = ConsoleService::get();
	ConsoleSessionView *view = Ec::current()->get_tls<ConsoleSessionView*>(Ec::TLS_PARAM);
	Consumer<Console::SendPacket> &cons = view->cons();
	for(Console::SendPacket *pk; (pk = cons.get()) != 0; cons.next()) {
		switch(pk->cmd) {
			case Console::WRITE:
				view->put(*pk);
				if(view->is_active())
					s->screen().paint(pk->x,pk->y,pk->character,pk->color);
				break;

			case Console::SCROLL:
				view->scroll();
				if(view->is_active())
					s->screen().scroll();
				break;
		}
	}
}
