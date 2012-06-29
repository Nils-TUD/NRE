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
#include "ConsoleService.h"

using namespace nul;

ForwardCycler<CPU::iterator,LockPolicyDefault<SpinLock> > ConsoleSessionView::_cpus(CPU::begin(),CPU::end());
// 0 and 1 is for boot and HV
uint ConsoleSessionView::_next_uid = 2;

ConsoleSessionView::ConsoleSessionView(ConsoleSessionData *sess,uint id,DataSpace *in_ds)
	: RCUObject(), DListItem(), _id(id), _uid(Atomic::add(&_next_uid,+1)), _pos(0), _in_ds(in_ds),
	  _out_ds(Screen::SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW),
	  _prod(in_ds,false,false), _sess(sess) {
	memset(reinterpret_cast<void*>(_out_ds.virt()),0,_out_ds.size());
}

ConsoleSessionView::~ConsoleSessionView() {
	delete _in_ds;
}

bool ConsoleSessionView::is_active() const {
	return _sess->is_active(this);
}

void ConsoleSessionView::swap() {
	_out_ds.switch_to(ConsoleService::get()->screen()->mem());
}
