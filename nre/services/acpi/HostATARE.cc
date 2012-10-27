/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2010, Bernhard Kauer <bk@vmmon.org>
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

#include "HostATARE.h"

using namespace nre;

void HostATARE::debug_show_items() {
	NamedRef *ref = _head;
	for(unsigned i = 0; ref; ref = ref->next, i++) {
		Serial::get().writef("at: %3d %p+%04x type %02x %.*s\n",
		                     i, ref->ptr, ref->len, ref->ptr[0], ref->namelen, ref->name);
	}
}

void HostATARE::debug_show_routing() {
	if(!search_ref(_head, 0, "_PIC", false))
		Serial::get().writef("at: APIC mode unavailable - no _PIC method\n");

	for(NamedRef *dev = _head; dev; dev = dev->next) {
		if(dev->ptr[0] == 0x82) {
			bdf_type bdf = get_device_bdf(_head, dev);
			Serial::get().writef("at: %04x:%02x:%02x.%x tag %p name %.*s\n",
			                     bdf >> 16, (bdf >> 8) & 0xff, (bdf >> 3) & 0x1f, bdf & 7, dev->ptr - 1,
			                     4, dev->name + dev->namelen - 4);
			for(PciRoutingEntry *p = dev->routing; p; p = p->next) {
				Serial::get().writef("at:\t  parent %p addr %02x_%x gsi %x\n",
				                     dev->ptr - 1, p->adr >> 16, p->pin, p->gsi);
			}
		}
	}
}

/**
 * Returns the number of bytes for this package len.
 */
size_t HostATARE::get_pkgsize_len(const uint8_t *data) {
	return ((data[0] & 0xc0) && (data[0] & 0x30)) ? 0 : 1 + (data[0] >> 6);
}

/**
 * Read the pkgsize.
 */
size_t HostATARE::read_pkgsize(const uint8_t *data) {
	size_t res = data[0] & 0x3f;
	for(size_t i = 1; i < get_pkgsize_len(data); i++)
		res += data[i] << (8 * i - 4);
	return res;
}

/**
 * Returns whether this is a nameseg.
 */
bool HostATARE::name_seg(const uint8_t *res) {
	for(size_t i = 0; i < 4; i++) {
		if(!((res[i] >= 'A' && res[i] <= 'Z') || (res[i] == '_')
		     || (i && res[i] >= '0' && res[i] <= '9')))
			return false;
	}
	return true;
}

/**
 * Get the length of the nameprefix;
 */
size_t HostATARE::get_nameprefix_len(const uint8_t *table) {
	const uint8_t *res = table;
	if(*res == 0x5c)
		res++;
	else {
		while(*res == 0x5e)
			res++;
	}

	if(*res == 0x2e)
		res++;
	else if(*res == 0x2f)
		res += 2;
	return res - table;
}

/**
 * Get the length of a name.
 *
 * Returns 0 on failure or the length.
 */
size_t HostATARE::get_name_len(const uint8_t *table) {
	const uint8_t *res = table;
	if(*res == 0x5c)
		res++;
	else {
		while(*res == 0x5e)
			res++;
	}

	if(*res == 0x2e) {
		if(name_seg(res + 1) && name_seg(res + 5))
			return res - table + 9;
	}
	else if(*res == 0x2f) {
		size_t i;
		for(i = 0; i < res[1]; i++) {
			if(!name_seg(res + 2 + i * 4))
				return 0;
		}
		if(i)
			return res - table + 2 + 4 * i;
	}
	else if(name_seg(res))
		return res - table + 4;
	return res - table;
}

/**
 * Calc an absname.
 */
void HostATARE::get_absname(NamedRef *parent, const uint8_t *name, size_t &namelen, char *res,
                            size_t skip) {
	size_t nameprefixlen = get_nameprefix_len(name);
	namelen = namelen - nameprefixlen;

	// we already have an absname?
	if(*name == 0x5c || !parent)
		memcpy(res, name + nameprefixlen, namelen);
	else {
		ssize_t parent_namelen = parent->namelen - skip * 4;
		if(parent_namelen < 0)
			parent_namelen = 0;
		for(size_t pre = 0; name[pre] == 0x5e && parent_namelen; pre++)
			parent_namelen -= 4;
		memcpy(res, parent->name, parent_namelen);
		memcpy(res + parent_namelen, name + nameprefixlen, namelen);
		namelen += parent_namelen;
	}
}

/**
 * Get the length of a data item and the data value.
 *
 * Note: We support only numbers here.
 */
uint HostATARE::read_data(const uint8_t *data, size_t &length) {
	switch(data[0]) {
		case 0:
			length = 1;
			return 0;
		case 1:
			length = 1;
			return 1;
		case 0xff:
			length = 1;
			return 0xffffffff;
		case 0x0a:
			length = 2;
			return data[1];
		case 0x0b:
			length = 3;
			return data[1] | (data[2] << 8);
		case 0x0c:
			length = 5;
			return data[1] | (data[2] << 8) | (data[3] << 16) | (data[4] << 24);
		default:
			length = 0;
			return 0;
	}
}

/**
 * Search and read a packet with 4 entries.
 */
ssize_t HostATARE::get_packet4(const uint8_t *table, ssize_t len, uint *x) {
	for(const uint8_t *data = table; data < table + len; data++) {
		if(data[0] == 0x12) {
			size_t pkgsize_len = get_pkgsize_len(data + 1);
			if(!pkgsize_len || data[1 + pkgsize_len] != 0x04)
				continue;

			size_t offset = 1 + pkgsize_len + 1;
			for(size_t i = 0; i < 4; i++) {
				size_t len;
				x[i] = read_data(data + offset, len);
				if(!len)
					return 0;
				offset += len;
			}
			return data - table;
		}
	}
	return 0;
}

/**
 * Searches for PCI routing information in some region and adds them to dev.
 */
void HostATARE::search_prt_direct(NamedRef *dev, NamedRef *ref) {
	size_t l;
	for(size_t offset = 0; offset < ref->len; offset += l) {
		uint x[4];
		ssize_t packet = get_packet4(ref->ptr + offset, ref->len - offset, x);
		l = 1;
		if(!packet)
			continue;

		l = packet + read_pkgsize(ref->ptr + offset + packet + 1);
		dev->routing = new PciRoutingEntry(dev->routing, x[0], x[1], x[3]);
	}
}

/**
 * Searches for PCI routing information by following references in
 * the PRT method and adds them to dev.
 */
void HostATARE::search_prt_indirect(NamedRef *head, NamedRef *dev, NamedRef *prt) {
	uint found = 0;
	size_t name_len;
	for(size_t offset = get_pkgsize_len(prt->ptr + 1); offset < prt->len; offset += name_len) {
		name_len = get_name_len(prt->ptr + offset);
		if(name_len) {
			// skip our name
			if(!found++)
				continue;

			char name[name_len + 1];
			memcpy(name, prt->ptr + offset, name_len);
			name[name_len] = 0;
			NamedRef *ref = search_ref(head, dev, name, true);
			if(ref)
				search_prt_direct(dev, ref);
		}
		else
			name_len = 1;
	}
}

/**
 * Return a single value of a namedef declaration.
 */
uint HostATARE::get_namedef_value(NamedRef *head, NamedRef *parent, const char *name) {
	NamedRef *ref = search_ref(head, parent, name, false);
	if(ref && ref->ptr[0] == 0x8) {
		size_t name_len = get_name_len(ref->ptr + 1);
		size_t len;
		uint x = read_data(ref->ptr + 1 + name_len, len);
		if(len)
			return x;
	}
	return 0;
}

/**
 * Search some reference per name, either absolute or relative to some parent.
 */
HostATARE::NamedRef *HostATARE::search_ref(NamedRef *head, NamedRef *parent, const char *name,
                                           bool upstream) {
	size_t slen = strlen(name);
	size_t plen = parent ? parent->namelen : 0;
	for(size_t skip = 0; skip <= plen / 4; skip++) {
		size_t n = slen;
		char output[slen + plen];
		get_absname(parent, reinterpret_cast<const uint8_t*>(name), n, output, skip);
		for(NamedRef *ref = head; ref; ref = ref->next) {
			if(ref->namelen == n && !memcmp(ref->name, output, n))
				return ref;
		}
		if(!upstream)
			break;
	}
	return 0;
}

/**
 * Return a single bdf for a device struct by combining different device properties.
 */
HostATARE::bdf_type HostATARE::get_device_bdf(NamedRef *head, NamedRef *dev) {
	uint adr = get_namedef_value(head, dev, "_ADR");
	uint bbn = get_namedef_value(head, dev, "_BBN");
	uint seg = get_namedef_value(head, dev, "_SEG");
	return (seg << 16) + (bbn << 8) + ((adr >> 16) << 3) + (adr & 0xffff);
}

/**
 * Add all named refs from a table and return the list head pointer.
 */
HostATARE::NamedRef *HostATARE::add_refs(const uint8_t *table, unsigned len, NamedRef *res) {
	for(const uint8_t *data = table; data < table + len; data++) {
		if((data[0] == 0x5b && data[1] == 0x82) // devices
		   || (data[0] == 0x08)      // named objects
		   || (data[0] == 0x10)      // scopes
		   || (data[0] == 0x14)      // methods
		   ) {
			if(data[0] == 0x5b)
				data++;

			bool has_pkgsize = (data[0] == 0x10) || (data[0] == 0x14) || (data[0] == 0x82);
			size_t pkgsize_len = 0;
			size_t pkgsize = 0;
			if(has_pkgsize) {
				pkgsize_len = get_pkgsize_len(data + 1);
				pkgsize = read_pkgsize(data + 1);
			}
			size_t name_len = get_name_len(data + 1 + pkgsize_len);
			if((has_pkgsize && !pkgsize_len) || !name_len || data + pkgsize > table + len)
				continue;

			// fix previous len
			if(res && !res->len)
				res->len = data - res->ptr;

			// search for the parent in this table
			NamedRef *parent = res;
			for(; parent; parent = parent->next) {
				if(parent->ptr < data && parent->ptr + parent->len > data)
					break;
			}

			// to get an absolute name
			char *name = new char[name_len + (parent ? parent->namelen : 0)];
			get_absname(parent, data + 1 + pkgsize_len, name_len, name);
			res = new NamedRef(res, data, pkgsize, name, name_len);

			// at least skip the header
			data += pkgsize_len;
		}
	}

	// fix len of last item
	if(res && !res->len)
		res->len = table + len - res->ptr;
	return res;
}

/**
 * Add the PCI routing information to the devices.
 */
void HostATARE::add_routing(NamedRef *head) {
	for(NamedRef *dev = head; dev; dev = dev->next) {
		if(dev->ptr[0] == 0x82) {
			NamedRef *prt = search_ref(head, dev, "_PRT", false);
			if(prt) {
				search_prt_direct(dev, prt);
				search_prt_indirect(head, dev, prt);
			}
		}
	}
}
