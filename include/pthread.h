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
#include <arch/SpinLock.h>

typedef uint32_t pthread_key_t;
typedef spinlock_t pthread_mutex_t;
typedef int pthread_t;
typedef int pthread_once_t;
typedef int pthread_mutexattr_t;

#ifdef __cplusplus
extern "C" {
#endif

int pthread_key_create(pthread_key_t* key, void (*f)(void*));
int pthread_key_delete(pthread_key_t key);
int pthread_cancel(pthread_t thread);
int pthread_once(pthread_once_t* control, void (*init)(void));
void* pthread_getspecific(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, void* data);
int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr);
int pthread_mutex_lock(pthread_mutex_t* mutex);
int pthread_mutex_unlock(pthread_mutex_t* mutex);

#ifdef __cplusplus
}
#endif
