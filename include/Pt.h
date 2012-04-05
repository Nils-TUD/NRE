/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

class Pt {
public:
	typedef unsigned pid_t;

public:
	explicit Pt();
	virtual ~Pt();

private:
	virtual void call(pid_t pid) = 0;

	static void start(pid_t pid,void *thiz,Utcb *utcb) {
		static_cast<Pt*>(thiz)->call(pid);
	}

private:
};
