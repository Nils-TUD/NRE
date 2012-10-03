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
#include <subsystem/ChildManager.h>
#include <util/SList.h>

class VMConfig;

nre::OStream &operator<<(nre::OStream &os,const VMConfig &cfg);

class VMConfig : public nre::SListItem {
	static const size_t MAX_ARGS_LEN	= 256;

	friend nre::OStream &operator<<(nre::OStream &os,const VMConfig &cfg);

	class Module : public nre::SListItem {
	public:
		explicit Module(const char *line,size_t namelen,size_t linelen)
			: nre::SListItem(), _name(line,namelen), _args(line,linelen) {
		}

		const nre::String &name() const {
			return _name;
		}
		const nre::String &args() const {
			return _args;
		}

	private:
		nre::String _name;
		nre::String _args;
	};

	class VMChildConfig : public nre::ChildConfig {
	public:
		explicit VMChildConfig(const nre::SList<Module> &mods)
			: nre::ChildConfig(0), _mods(mods) {
		}

		virtual bool has_module_access(size_t i) const {
			return get_module(i) != _mods.end();
		}
		virtual const char *module_cmdline(size_t i) const {
			nre::SList<Module>::iterator mod = get_module(i);
			return mod->args().str();
		}

	private:
		nre::SList<Module>::iterator get_module(size_t i) const {
			nre::Hip::mem_iterator mem = nre::Hip::get().mem_begin() + i;
			for(nre::SList<Module>::iterator it = _mods.begin(); it != _mods.end(); ++it) {
				if(strcmp(mem->cmdline(),it->name().str()) == 0)
					return it;
			}
			return _mods.end();
		}

		const nre::SList<Module> &_mods;
	};

public:
	explicit VMConfig(uintptr_t phys,size_t size,const char *name)
		: nre::SListItem(), _ds(size,nre::DataSpaceDesc::ANONYMOUS,nre::DataSpaceDesc::R,phys),
		  _name(name), _mods() {
		find_mods(size);
	}
	~VMConfig() {
		for(nre::SList<Module>::iterator it = _mods.begin(); it != _mods.end(); ++it)
			delete &*it;
	}

	const char *name() const {
		return _name;
	}
	const nre::String &args() const {
		nre::SList<Module>::iterator first = _mods.begin();
		return first->args();
	}
	nre::Child::id_type start(nre::ChildManager &cm,size_t console,cpu_t cpu);

private:
	void find_mods(size_t len);
	static nre::Hip::mem_iterator get_module(const nre::String &name);

	nre::DataSpace _ds;
	const char *_name;
	nre::SList<Module> _mods;
};
