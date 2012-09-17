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

#pragma once

#include <services/Console.h>
#include <services/SysInfo.h>

class SysInfoPage {
public:
	enum {
		MAX_NAME_LEN	= 16,
		ROWS			= nre::Console::ROWS - 3,
		DATA_ROWS		= ROWS - 2
	};

	explicit SysInfoPage(nre::ConsoleSession &cons,nre::SysInfoSession &sysinfo)
		: _top(0), _cons(cons), _sysinfo(sysinfo), _sm() {
	}
	virtual ~SysInfoPage() {
	}

	size_t top() const {
		return _top;
	}
	void top(size_t top) {
		_top = top;
	}

	virtual void refresh_console(bool update) = 0;

protected:
	void display_footer(nre::ConsoleStream &cs,size_t i) {
		cs.pos(0,nre::Console::ROWS - 1);
		cs.color(i == 0 ? 0x17 : 0x71);
		cs.writef("%*s",nre::Console::COLS / 2,"Scs");
		cs.color(i == 1 ? 0x17 : 0x71);
		cs.writef("%*s",nre::Console::COLS / 2,"Pds");
	}

	const char *getname(const nre::String &name,size_t &len) {
		// don't display the path to the program (might be long) and cut off arguments
		size_t lastslash = 0,end = name.length();
		for(size_t i = 0; i < name.length(); ++i) {
			if(name.str()[i] == '/')
				lastslash = i + 1;
			if(name.str()[i] == ' ') {
				end = i;
				break;
			}
		}
		len = end - lastslash;
		return name.str() + lastslash;
	}

	size_t _top;
	nre::ConsoleSession &_cons;
	nre::SysInfoSession &_sysinfo;
	nre::UserSm _sm;
};

class ScInfoPage : public SysInfoPage {
	enum {
		MAX_TIME_LEN	= 6,
		MAX_SUMTIME_LEN	= 12,
	};
public:
	explicit ScInfoPage(nre::ConsoleSession &cons,nre::SysInfoSession &sysinfo)
		: SysInfoPage(cons,sysinfo) {
	}
	virtual void refresh_console(bool update);
};

class PdInfoPage : public SysInfoPage {
public:
	explicit PdInfoPage(nre::ConsoleSession &cons,nre::SysInfoSession &sysinfo)
		: SysInfoPage(cons,sysinfo) {
	}
	virtual void refresh_console(bool update);
};
