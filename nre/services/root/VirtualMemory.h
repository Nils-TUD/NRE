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

#include <kobj/UserSm.h>
#include <util/RegionManager.h>
#include <util/ScopedLock.h>

/**
 * The virtual memory is split in two areas: everything from <end-of-data-area> to RAM_BEGIN is
 * used to map arbitrary stuff (e.g. multiboot-modules or device memory). The area from RAM_BEGIN
 * to RAM_END is used for RAM. This is mapped at the beginning and never unmapped.
 */
class VirtualMemory {
    static const uintptr_t RAM_BEGIN    = ARCH_REGIONS_END;
    static const uintptr_t RAM_END      = ARCH_KERNEL_START;

public:
    /**
     * Only for the startup: Allocate virtual memory for RAM
     *
     * @param phys the physical address
     * @param size the size (might be adjusted)
     * @return the virtual address
     */
    static uintptr_t alloc_ram(uintptr_t phys, size_t &size) {
        size = nre::Math::round_up<size_t>(size, nre::ExecEnv::PAGE_SIZE);
        if(RAM_BEGIN + phys >= RAM_END)
            return 0;
        if(RAM_BEGIN + phys + size > RAM_END)
            size = RAM_END - (RAM_BEGIN + phys);
        _ramend = nre::Math::max(_ramend, RAM_BEGIN + phys + size);
        return RAM_BEGIN + phys;
    }
    /**
     * @return the virtual address for <phys>
     */
    static uintptr_t phys_to_virt(uintptr_t phys) {
        return phys + RAM_BEGIN;
    }
    /**
     * @return the physical address for <virt>
     */
    static uintptr_t virt_to_phys(uintptr_t virt) {
        return virt - RAM_BEGIN;
    }

    /**
     * @return the beginning of all RAM (virtual address)
     */
    static uintptr_t ram_begin() {
        return RAM_BEGIN;
    }
    /**
     * @return the end of available RAM (virtual address)
     */
    static uintptr_t ram_end() {
        return _ramend;
    }

    /**
     * @return the amount of used virtual memory
     */
    static size_t used() {
        return _used;
    }
    /**
     * Allocates <size> bytes from the free virtual memory
     */
    static uintptr_t alloc(size_t size, size_t align = 1) {
        nre::ScopedLock<nre::UserSm> guard(&_sm);
        size = nre::Math::round_up<size_t>(size, nre::ExecEnv::PAGE_SIZE);
        uintptr_t addr = _regs.alloc(size, align);
        _used += size;
        return addr;
    }
    /**
     * Free the <size> bytes @ <addr>
     */
    static void free(uintptr_t addr, size_t size) {
        nre::ScopedLock<nre::UserSm> guard(&_sm);
        size = nre::Math::round_up<size_t>(size, nre::ExecEnv::PAGE_SIZE);
        _regs.free(addr, size);
        _used -= size;
    }

    /**
     * @return the virtual memory regions
     */
    static const nre::RegionManager<> &regions() {
        return _regs;
    }

private:
    VirtualMemory();

    static nre::RegionManager<> _regs;
    static nre::UserSm _sm;
    static VirtualMemory _init;
    static size_t _used;
    static size_t _ramend;
};
