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

#include <arch/SpinLock.h>
#include <kobj/Thread.h>
#include <pthread.h>
#include <cstdlib>
#include <util/Atomic.h>

using namespace nre;

int pthread_key_create(pthread_key_t* key,void (*)(void*)) {
	*key = Thread::current()->create_tls();
	return 0;
}

int pthread_key_delete(pthread_key_t) {
	// TODO
	return 0;
}

int pthread_cancel(pthread_t) {
	// TODO
	return 0;
}

int pthread_once(pthread_once_t* control,void (*init)(void)) {
	if(Atomic::swap(control,1,2))
		(*init)();
	return 0;
}

void* pthread_getspecific(pthread_key_t key) {
	return Thread::current()->get_tls<void*>(key);
}

int pthread_setspecific(pthread_key_t key,void* data) {
	Thread::current()->set_tls<void*>(key,data);
	return 0;
}

int pthread_mutex_init(pthread_mutex_t* mutex,const pthread_mutexattr_t*) {
	*mutex = 0;
	return 0;
}

int pthread_mutex_lock(pthread_mutex_t* mutex) {
	// TODO a spinlock is not really optimal here
	lock(mutex);
	return 0;
}

int pthread_mutex_unlock(pthread_mutex_t* mutex) {
	unlock(mutex);
	return 0;
}
