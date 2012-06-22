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

#include "ConsoleSessionData.h"

using namespace nul;

ConsoleSessionData::ConsoleSessionData(Service *s,size_t id,capsel_t caps,Pt::portal_func func)
	: SessionData(s,id,caps,func), _view(0), _sm(), _ec(receiver,next_cpu()), _sc(),
	  _in_ds(), _out_ds(),
	  _buffer(Screen::VIEW_COUNT * ExecEnv::PAGE_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW),
	  _prod(), _cons() {
	_ec.set_tls<capsel_t>(Ec::TLS_PARAM,caps);
	memset(reinterpret_cast<void*>(_buffer.virt()),0,_buffer.size());
}

ConsoleSessionData::~ConsoleSessionData() {
	delete _out_ds;
	delete _in_ds;
	delete _prod;
	delete _cons;
	delete _sc;
}

void ConsoleSessionData::invalidate() {
	if(_cons)
		_cons->stop();
}

void ConsoleSessionData::put(const Console::SendPacket &pk) {
	if(pk.y < Screen::ROWS && pk.x < Screen::COLS && pk.view < Screen::VIEW_COUNT) {
		uint8_t *buf = reinterpret_cast<uint8_t*>(_buffer.virt()) + pk.view * ExecEnv::PAGE_SIZE;
		uint8_t *pos = buf + pk.y * Screen::COLS * 2 + pk.x * 2;
		*pos = pk.character;
		*(pos + 1) = pk.color;
	}
}

void ConsoleSessionData::scroll(uint view) {
	if(view < Screen::VIEW_COUNT) {
		char *screen = reinterpret_cast<char*>(_buffer.virt()) + view * ExecEnv::PAGE_SIZE;
		memmove(screen,screen + Screen::COLS * 2,Screen::COLS * (Screen::ROWS - 1) * 2);
		memset(screen + Screen::COLS * (Screen::ROWS - 1) * 2,0,Screen::COLS * 2);
	}
}

void ConsoleSessionData::switch_to(uint view) {
	assert(view < Screen::VIEW_COUNT);
	ConsoleService *s = ConsoleService::get();
	ScopedLock<UserSm> guard(&_sm);
	_view = view;
	s->screen().set_view(view);
	uint8_t *buf = reinterpret_cast<uint8_t*>(_buffer.virt()) + view * ExecEnv::PAGE_SIZE;
	for(uint8_t y = 0; y < Screen::ROWS; ++y) {
		for(uint8_t x = 0; x < Screen::COLS; ++x) {
			uint8_t *pos = buf + y * Screen::COLS * 2 + x * 2;
			s->screen().paint(x,y,*pos,*(pos + 1));
		}
	}
}

void ConsoleSessionData::accept_ds(DataSpace *ds) {
	ScopedLock<UserSm> guard(&_sm);
	if(_out_ds == 0) {
		_out_ds = ds;
		_prod = new Producer<Console::ReceivePacket>(ds,false,false);
	}
	else if(_in_ds == 0) {
		_in_ds = ds;
		_cons = new Consumer<Console::SendPacket>(ds,false);
		_sc = new Sc(&_ec,Qpd());
		_sc->start();
	}
	else
		throw ServiceException(E_ARGS_INVALID);
}

void ConsoleSessionData::receiver(void *) {
	capsel_t caps = Ec::current()->get_tls<word_t>(Ec::TLS_PARAM);
	ScopedLock<RCULock> guard(&RCU::lock());
	ConsoleService *s = ConsoleService::get();
	ConsoleSessionData *sess = s->get_session<ConsoleSessionData>(caps);
	Consumer<Console::SendPacket> *cons = sess->cons();
	while(1) {
		Console::SendPacket *pk = cons->get();
		if(pk == 0)
			return;
		{
			ScopedLock<UserSm> guard(&sess->sm());
			switch(pk->cmd) {
				case Console::WRITE:
					sess->put(*pk);
					if(sess == s->active() && sess->view() == pk->view)
						s->screen().paint(pk->x,pk->y,pk->character,pk->color);
					break;

				case Console::SCROLL:
					sess->scroll(pk->view);
					if(sess == s->active() && sess->view() == pk->view)
						s->screen().scroll();
					break;
			}
		}
		cons->next();
	}
}
