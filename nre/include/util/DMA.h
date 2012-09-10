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

#include <mem/DataSpace.h>
#include <stream/OStream.h>
#include <utcb/UtcbFrame.h>
#include <Assert.h>

namespace nre {

struct DMADesc {
	size_t offset;
	size_t count;

	explicit DMADesc() : offset(), count() {
	}
	explicit DMADesc(size_t offset,size_t count) : offset(offset), count(count) {
	}
};

template<size_t MAX>
class DMADescList {
public:
	typedef const DMADesc *iterator;

	explicit DMADescList() : _descs(), _count(0), _total(0) {
	}

	size_t count() const {
		return _count;
	}
	iterator begin() const {
		return _descs;
	}
	iterator end() const {
		return _descs + _count;
	}

	size_t bytecount() const {
		return _total;
	}

	void add(const DMADesc &desc) {
		_descs[_count++] = desc;
		_total += desc.count;
	}
	void clear() {
		_count = _total = 0;
	}

	bool in(void *dst,size_t len,size_t offset,const DataSpace &src) const {
		return inout(dst,len,offset,src,false);
	}
	bool out(const void *src,size_t len,size_t offset,const DataSpace &dst) const {
		return inout(const_cast<void*>(src),len,offset,dst,true);
	}

private:
	bool inout(void *buffer,size_t len,size_t offset,const DataSpace &ds,bool out) const {
		iterator it;
		char *buf = reinterpret_cast<char*>(buffer);
		for(it = begin(); it != end() && offset >= it->count; ++it)
			offset -= it->count;
		for(; len > 0 && it != end(); ++it) {
			assert(it->count >= offset);
			size_t sublen = Math::min<size_t>(it->count - offset,len);
			if((it->offset + offset) > ds.size() || (it->offset + offset + sublen) > ds.size())
				break;

			char *addr = reinterpret_cast<char*>(it->offset + ds.virt() + offset);
			if(out)
				memcpy(addr,buf,sublen);
			else
				memcpy(buf,addr,sublen);

			buf += sublen;
			len -= sublen;
			offset = 0;
		}
		return len > 0;
	}

	DMADesc _descs[MAX];
	size_t _count;
	size_t _total;
};

template<size_t MAX>
static inline OStream &operator<<(OStream &os,const DMADescList<MAX> &l) {
	os << "DMADescList[" << l.count() << ", " << l.bytecount() << "] (";
	for(typename DMADescList<MAX>::iterator it = l.begin(); it != l.end(); ) {
		os << it->offset << ":" << it->count;
		if(++it != l.end())
			os << ",";
	}
	os << ")";
	return os;
}

template<size_t MAX>
static inline UtcbFrameRef &operator<<(UtcbFrameRef &uf,const DMADescList<MAX> &l) {
	uf << l.count();
	for(typename DMADescList<MAX>::iterator it = l.begin(); it != l.end(); ++it)
		uf << *it;
	return uf;
}
template<size_t MAX>
static inline UtcbFrameRef &operator>>(UtcbFrameRef &uf,DMADescList<MAX> &l) {
	size_t count;
	uf >> count;
	l.clear();
	while(count-- > 0) {
		DMADesc desc;
		uf >> desc;
		l.add(desc);
	}
	return uf;
}

}
