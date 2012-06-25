/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

namespace nul {

class LockPolicyNone {
public:
	explicit LockPolicyNone() {
	}
	virtual ~LockPolicyNone() {
	}
	void lock() {
	}
	void unlock() {
	}

private:
	LockPolicyNone(const LockPolicyNone&);
	LockPolicyNone& operator=(const LockPolicyNone&);
};

template<class T>
class LockPolicyDefault {
public:
	explicit LockPolicyDefault() : _l() {
	}
	virtual ~LockPolicyDefault() {
	}
	void lock() {
		_l.down();
	}
	void unlock() {
		_l.up();
	}

private:
	LockPolicyDefault(const LockPolicyDefault<T>&);
	LockPolicyDefault<T>& operator=(const LockPolicyDefault<T>&);

private:
	T _l;
};

}
