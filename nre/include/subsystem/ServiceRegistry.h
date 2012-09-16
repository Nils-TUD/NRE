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

#include <subsystem/Child.h>
#include <util/BitField.h>
#include <util/SList.h>
#include <Exception.h>

namespace nre {

class ServiceRegistryException : public Exception {
public:
	DEFINE_EXCONSTRS(ServiceRegistryException)
};

/**
 * Keeps track of registered services, i.e. stores the child that registered it, the name, on
 * which CPUs its available and the portal capabilities.
 */
class ServiceRegistry {
public:
	/**
	 * A service in the registry
	 */
	class Service : public SListItem {
		friend class ServiceRegistry;

		/**
		 * Constructor
		 *
		 * @param child the child that created the service
		 * @param name the name of the service
		 * @param pts the start of the portal capability selectors
		 * @param count the number of selectors
		 * @param available bitfield that specifies on what CPUs its available
		 */
		explicit Service(Child *child,const String &name,capsel_t pts,size_t count,
				const BitField<Hip::MAX_CPUS> &available)
			: SListItem(), _child(child), _name(name), _pts(pts), _count(count), _sm(0),
			  _available(available) {
		}
		/**
		 * The destructor revokes the caps and frees the selectors
		 */
		~Service() {
			CapRange(_pts,_count,Crd::OBJ_ALL).revoke(true);
			CapSelSpace::get().free(_pts,_count);
		}

	public:
		/**
		 * @return the child that created the service
		 */
		Child *child() const {
			return _child;
		}
		/**
		 * @return the name of the service
		 */
		const String &name() const {
			return _name;
		}
		/**
		 * @return the bitfield that specifies on what CPUs the service can be used
		 */
		const BitField<Hip::MAX_CPUS> &available() const {
			return _available;
		}
		/**
		 * @return the base of the capability selectors for this service
		 */
		capsel_t pts() const {
			return _pts;
		}
		/**
		 * A semaphore that can be used to notify the service about potentially destroyed sessions
		 */
		Sm &sm() {
			return _sm;
		}
		const Sm &sm() const {
			return _sm;
		}

	private:
		Child *_child;
		String _name;
		capsel_t _pts;
		size_t _count;
		Sm _sm;
		BitField<Hip::MAX_CPUS> _available;
	};

	typedef SListIterator<Service> iterator;

	/**
	 * Constructor
	 */
	explicit ServiceRegistry() : _srvs() {
	}

	/**
	 * @return beginning of services
	 */
	iterator begin() const {
		return _srvs.begin();
	}
	/**
	 * @return end of services
	 */
	iterator end() const {
		return _srvs.end();
	}

	/**
	 * Registers the given service
	 *
	 * @param child the child that created the service
	 * @param name the name of the service
	 * @param pts the start of the portal capability selectors
	 * @param count the number of selectors
	 * @param available bitfield that specifies on what CPUs its available
	 * @return the created service
	 * @throws ServiceRegistryException if the service does already exist
	 */
	const Service* reg(Child *child,const String &name,capsel_t pts,size_t count,
			const BitField<Hip::MAX_CPUS> &available) {
		if(search(name))
			throw ServiceRegistryException(E_EXISTS,64,"Service '%s' does already exist",name.str());
		Service *s = new Service(child,name,pts,count,available);
		_srvs.append(s);
		return s;
	}
	/**
	 * Unregisters the service with given name from given child. Note that only the created can
	 * unregister it.
	 *
	 * @param child the child that wants to unregister it
	 * @param name the name of the service
	 * @throws ServiceRegistryException if the service doesn't exist or doesn't belong to <child>
	 */
	void unreg(Child *child,const String &name) {
		Service *s = search(name);
		if(!s)
			throw ServiceRegistryException(E_NOT_FOUND,64,"Service '%s' does not exist",name.str());
		if(s->child() != child) {
			throw ServiceRegistryException(E_NOT_FOUND,128,"Child '%s' does not own service '%s'",
					child->cmdline().str(),name.str());
		}
		_srvs.remove(s);
		delete s;
	}

	/**
	 * @param name the service name
	 * @return the service with given name
	 */
	const Service* find(const String &name) const {
		return search(name);
	}
	/**
	 * Removes all services from the given child
	 *
	 * @param child the child
	 */
	void remove(Child *child) {
		for(iterator it = _srvs.begin(); it != _srvs.end(); ) {
			if(it->child() == child) {
				_srvs.remove(&*it);
				delete &*it;
				it = _srvs.begin();
			}
			else
				++it;
		}
	}

private:
	Service *search(const String &name) const {
		for(iterator it = _srvs.begin(); it != _srvs.end(); ++it) {
			if(it->name() == name)
				return &*it;
		}
		return 0;
	}

	SList<Service> _srvs;
};

}
