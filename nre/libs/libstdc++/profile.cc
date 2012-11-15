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

#include <arch/Startup.h>
#include <arch/SpinLock.h>
#include <kobj/Thread.h>
#include <util/ScopedLock.h>
#include <util/Math.h>
#include <Compiler.h>
#include <Assert.h>
#include <Hip.h>

#define MAX_EVENTS  1024
#define STACK_SIZE  1024
#define NOINSTR     __attribute__ ((no_instrument_function))

#ifdef PROFILE
struct ProfileEvent {
    enum {ENTER, LEAVE} type;
    int tid;
    uintptr_t func;
    timevalue_t time;
    ProfileEvent *next;
};

NOINSTR static uint8_t inb(uint16_t port);
NOINSTR static void outb(uint16_t port, uint8_t value);
NOINSTR static timevalue_t rdtsc();
NOINSTR static void logc(char c);
NOINSTR static void logu(ulong u, uint base);
NOINSTR static void init();
NOINSTR static ProfileEvent *alloc();

EXTERN_C NOINSTR void __cyg_profile_func_enter(void *this_fn, void *call_site);
EXTERN_C NOINSTR void __cyg_profile_func_exit(void *this_fn, void *call_site);

static ProfileEvent *evfree = nullptr;
static ProfileEvent *evlist = nullptr;
static ProfileEvent *evend = nullptr;
static ProfileEvent evbuf[MAX_EVENTS];
static nre::SpinLock spin;

static bool inited = false;
static size_t stackPos = 0;
static bool inProf = false;
static timevalue_t callStack[STACK_SIZE];

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    asm volatile ("inb %1, %0" : "=a" (val) : "d" (port));
    return val;
}
static inline void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a" (value), "d" (port));
}

static inline timevalue_t rdtsc() {
    uint32_t u, l;
    asm volatile ("rdtsc" : "=a" (l), "=d" (u));
    return (timevalue_t)u << 32 | l;
}

static void logc(char c) {
    outb(0x3f8, c);
    while((inb(0x3fd) & 0x20) == 0)
        ;
}
static void logu(ulong u, uint base) {
    if(u >= base)
        logu(u / base, base);
    logc("0123456789ABCDEF"[(u % base)]);
}

static void init() {
    evfree = nullptr;
    for(size_t i = 0; i < MAX_EVENTS; ++i) {
        evbuf[i].next = evfree;
        evfree = evbuf + i;
    }
    inited = true;
}

static ProfileEvent *alloc() {
    nre::ScopedLock<nre::SpinLock> guard(&spin);
    ProfileEvent *ev = evfree;
    if(!ev) {
        ev = evlist;
        while(ev) {
            logc(ev->type == ProfileEvent::ENTER ? '>' : '<');
            logu(ev->tid, 10);
            logc(':');
            if(ev->type == ProfileEvent::ENTER)
                logu(ev->func, 16);
            else
                logu(ev->time, 10);
            logc('\n');
            ev = ev->next;
        }
        evfree = evlist;
        evlist = nullptr;
        evend = nullptr;
        ev = evfree;
    }
    evfree = evfree->next;
    ev->next = nullptr;
    if(evend)
        evend->next = ev;
    else
        evlist = ev;
    evend = ev;
    return ev;
}

void __cyg_profile_func_enter(void *this_fn, UNUSED void *call_site) {
    if(inProf || _startup_info.child || !_startup_info.done)
        return;
    if(!inited)
        init();
    inProf = true;
    ProfileEvent *ev = alloc();
    ev->type = ProfileEvent::ENTER;
    ev->tid = ((ulong) nre::Thread::current()->utcb() >> 12) % 64;
    ev->func = (ulong)this_fn;
    ev->time = rdtsc();
    callStack[stackPos++] = ev->time;
    inProf = false;
}

void __cyg_profile_func_exit(UNUSED void *this_fn, UNUSED void *call_site) {
    timevalue_t now = rdtsc();
    if(inProf || _startup_info.child || !_startup_info.done || stackPos <= 0)
        return;
    inProf = true;
    timevalue_t lastsched = nre::Hip::get().last_sched;
    ProfileEvent *ev = alloc();
    stackPos--;
    ev->type = ProfileEvent::LEAVE;
    ev->tid = ((ulong) nre::Thread::current()->utcb() >> 12) % 64;
    if(lastsched > callStack[stackPos])
        ev->time = nre::Math::min<timevalue_t>((now - lastsched) * 2, now - callStack[stackPos]);
    else
        ev->time = now - callStack[stackPos];
    inProf = false;
}

#endif
