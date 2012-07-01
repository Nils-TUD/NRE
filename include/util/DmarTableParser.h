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

#include <arch/Types.h>
#include <Assert.h>

namespace nul {

class DmarTableParser {
public:
	enum Type {
		DHRD = 0,
		RMRR = 1,
		ATSR = 2,
		RHSA = 3,
	};

	enum ScopeType {
		MSI_CAPABLE_HPET = 4,
	};

	class DeviceScope {
	public:
		DeviceScope(const char *base,size_t size_left) : _base(base), _size_left(size_left) {
			//printf("%p len = %u (%u), type = %u\n", base, _elem->length, size_left, _elem->type);
		}

		uint8_t id() const {
			return _elem->id;
		}
		ScopeType type() const {
			return ScopeType(_elem->type);
		}

		uint16_t rid() {
			// We don't know what to do if this is not plain PCI.
			if(_elem->length <= 6)
				return 0;
			assert(_elem->length - 6 == 2);
			return (_elem->start_bus << 8) | (_elem->path[0] << 3) | _elem->path[1];
		}

		bool has_next() const {
			return _size_left > _elem->length;
		}

		DeviceScope next() {
			assert(has_next());
			assert(_elem->length > 0);
			return DeviceScope(_base + _elem->length,_size_left - _elem->length);
		}

	private:
		union {
			const char *_base;
			struct PACKED {
				uint8_t type;
				uint8_t length;
				uint16_t _res;
				uint8_t id;
				uint8_t start_bus;
				uint8_t path[];
			}*_elem;
		};
		size_t _size_left;
	};

	class Dhrd {
	public:
		Dhrd(const char *base,size_t size_left) : _base(base), _size_left(size_left) {
		}

		uint8_t flags() const {
			return _elem->flags;
		}
		uint16_t segment() const {
			return _elem->segment;
		}
		uint64_t base() const {
			return _elem->base;
		}

		bool has_scopes() {
			return (_size_left - (_elem->scope - _base)) >= 6;
		}

		DeviceScope get_scope() {
			return DeviceScope(_elem->scope,_size_left - (_elem->scope - _base));
		}

	private:
		union {
			const char *_base;
			struct PACKED {
				uint8_t flags;
				uint8_t _res;
				uint16_t segment;
				uint64_t base;

				char scope[];
			}*_elem;
		};
		size_t _size_left;
	};

	class Element {
	public:
		Element(const char *base,size_t size_left) : _base(base), _size_left(size_left) {
		}

		Type type() const {
			return Type(_elem->type);
		}
		bool has_next() const {
			return _size_left > _elem->length;
		}

		Element next() {
			assert(has_next());
			assert(_elem->length > 0);
			return Element(_base + _elem->length,_size_left - _elem->length);
		}

		Dhrd get_dhrd() {
			assert(_elem->type == DHRD);
			return Dhrd(_base + 4,_elem->length - 4);
		}

	private:
		union {
			const char *_base;
			const struct PACKED {
				uint16_t type;
				uint16_t length;
			}*_elem;
		};
		size_t _size_left;
	};

	Element get_element() {
		return Element(_base + 48,_header->length - 48);
	}

	DmarTableParser(const char *base) : _base(base) {
		assert(_header->signature == 0x52414d44U); // "DMAR"
	}

private:
	union {
		const char *_base;
		const struct PACKED {
			uint32_t signature; // DMAR
			uint32_t length;
			// ...
		}*_header;
	};
};

}
