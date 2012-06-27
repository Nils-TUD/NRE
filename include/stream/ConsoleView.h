/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <stream/OStream.h>
#include <stream/IStream.h>
#include <service/Connection.h>
#include <service/Session.h>
#include <dev/Console.h>
#include <cstring>

namespace nul {

class ConsoleView : public IStream, public OStream {
	enum {
		IN_DS_SIZE	= ExecEnv::PAGE_SIZE,
		OUT_DS_SIZE	= ExecEnv::PAGE_SIZE,
	};

public:
	ConsoleView(ConsoleSession &sess)
		: _sess(sess), _view(0), _col(0), _row(0),
		  _in_ds(IN_DS_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW),
		  _out_ds(OUT_DS_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW),
		  _consumer(&_in_ds,true), _producer(&_out_ds,true,true) {
		create_view();
	}
	virtual ~ConsoleView() {
		destroy_view();
	}

	uint id() const {
		return _view;
	}

	Console::ReceivePacket receive() {
		Console::ReceivePacket *pk = _consumer.get();
		if(!pk)
			throw Exception(E_ABORT);
		Console::ReceivePacket res = *pk;
		_consumer.next();
		return res;
	}

	virtual char read();
	virtual void write(char c);

private:
	void scroll();

	void create_view() {
		UtcbFrame uf;
		uf << Console::CREATE_VIEW << _in_ds.desc() << _out_ds.desc();
		uf.delegate(_in_ds.sel(),0);
		uf.delegate(_out_ds.sel(),1);
		_sess.pt(CPU::current().id).call(uf);
		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw Exception(res);
		uf >> _view;
	}
	void destroy_view() {
		UtcbFrame uf;
		uf << Console::DESTROY_VIEW << _view;
		_sess.pt(CPU::current().id).call(uf);
	}

	ConsoleSession &_sess;
	uint _view;
	uint8_t _col;
	uint8_t _row;
	DataSpace _in_ds;
	DataSpace _out_ds;
	Consumer<Console::ReceivePacket> _consumer;
	Producer<Console::SendPacket> _producer;
};

}
