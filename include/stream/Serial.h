/*
 * TODO comment me
 *
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NUL.
 *
 * NUL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NUL is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <stream/OStream.h>
#include <arch/ExecEnv.h>

class Log;

namespace nul {

// note that we try to include as few headers as possible, because this class is used for debugging.
// that is, we lack debugging support in every header that we include since we can't include this
// header there.

class Connection;
class LogSession;

class BaseSerial : public OStream {
	friend class ::Log;

protected:
	enum {
		MAX_LINE_LEN	= 120
	};

public:
	static BaseSerial& get() {
		return *_inst;
	}

	explicit BaseSerial() : OStream() {
	}

protected:
	static BaseSerial *_inst;
};

class Serial : public BaseSerial {
	class Init {
		explicit Init();
		static Init init;
	};

	explicit Serial();
	virtual ~Serial();

	virtual void write(char c);

	Connection *_con;
	LogSession *_sess;
	size_t _bufpos;
	char _buf[MAX_LINE_LEN + 1];
};

}
