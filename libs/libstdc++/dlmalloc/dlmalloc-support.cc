// -*- Mode: C++ -*-

#include <cap/CapSpace.h>
#include <mem/DataSpace.h>
#include <kobj/Pd.h>
#include <stream/Serial.h>
#include <cstring>
#include <Syscalls.h>
#include <Atomic.h>
#include "dlmalloc-config.h"

using namespace nul;

typedef void *(*malloc_func)(size_t);
typedef void *(*realloc_func)(void*,size_t);
typedef void (*free_func)(void*);

EXTERN_C void* dlmemalign(size_t,size_t);
EXTERN_C void* dlmalloc(size_t);
EXTERN_C void* dlrealloc(void*,size_t);
EXTERN_C void dlfree(void*);

EXTERN_C void dlmalloc_init();
EXTERN_C void dlmalloc_init_locks(void);
EXTERN_C void* malloc(size_t);
EXTERN_C void* realloc(void*,size_t);
EXTERN_C void free(void*);

static void* startup_malloc(size_t size);
static void startup_free(void *ptr);

static malloc_func malloc_ptr = startup_malloc;
static realloc_func realloc_ptr = 0;
static free_func free_ptr = startup_free;

static char startup_heap[512];
static size_t pos = 0;

// Semaphore glue

void semaphore_init(DlMallocSm *lk,unsigned initial) {
	lk->sm = CapSpace::get().allocate();
	Syscalls::create_sm(lk->sm,0,Pd::current()->sel());
	lk->value = initial;
}

void semaphore_destroy(DlMallocSm *lk) {
	Syscalls::revoke(Crd(lk->sm,0,Crd::OBJ_ALL),true);
	CapSpace::get().free(lk->sm);
}

void semaphore_down(DlMallocSm *lk) {
	if(Atomic::xadd(&lk->value,-1) <= 0)
		Syscalls::sm_ctrl(lk->sm,Syscalls::SM_DOWN);
}

void semaphore_up(DlMallocSm *lk) {
	if(Atomic::xadd(&lk->value,+1) < 0)
		Syscalls::sm_ctrl(lk->sm,Syscalls::SM_UP);
}

// Backend allocator

void *mmap(void *,size_t size,int prot,int,int,off_t) {
	DataSpaceDesc desc(size,DataSpaceDesc::ANONYMOUS,prot);
	DataSpace::create(desc);
	memset(reinterpret_cast<void*>(desc.virt()),0,desc.size());
	return reinterpret_cast<void*>(desc.virt());
}

int munmap(void *start,size_t size) {
	Serial::get().writef("Leaking memory at %p+%zx\n",start,size);
	return 0;
}

// External interface

void dlmalloc_init() {
	dlmalloc_init_locks();
	malloc_ptr = dlmalloc;
	realloc_ptr = dlrealloc;
	free_ptr = dlfree;
}

void* malloc(size_t size) {
	return malloc_ptr(size);
}
void* realloc(void *p,size_t size) {
	return realloc_ptr(p,size);
}
void free(void *p) {
	char *addr = reinterpret_cast<char*>(p);
	if(addr >= startup_heap && addr < startup_heap + sizeof(startup_heap))
		startup_free(p);
	else
		free_ptr(p);
}

// startup malloc implementation

static void* startup_malloc(size_t size) {
	void* res;
	if(pos + size >= sizeof(startup_heap))
		return 0;

	res = startup_heap + pos;
	pos += size;
	return res;
}

static void startup_free(void *) {
	// we don't need to free that
}

// EOF
