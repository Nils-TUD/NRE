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

namespace nre {

// note that we try to include as few headers as possible, because this class is used for debugging.
// that is, we lack debugging support in every header that we include since we can't include this
// header there.

class Connection;
class LogSession;

/**
 * Common base class for all serial-outstreams. Should not be used directly.
 */
class BaseSerial : public OStream {
	friend class ::Log;

protected:
	enum {
		MAX_LINE_LEN	= 120
	};

	explicit BaseSerial() : OStream() {
	}

	static BaseSerial *_inst;
};

/**
 * Serial outstream for all tasks except root. Uses a buffer to keap at most one line or
 * MAX_LINE_LEN local until it is sent to the log-service via a portal call.
 */
class Serial : public BaseSerial {
	class Init {
		explicit Init();
		static Init init;
	};

public:
	/**
	 * @return the instance of the serial outstream
	 */
	static BaseSerial& get() {
		return *_inst;
	}

private:
	explicit Serial();
	virtual ~Serial();

	virtual void write(char c);

	Connection *_con;
	LogSession *_sess;
	size_t _bufpos;
	char _buf[MAX_LINE_LEN + 1];
};

}
