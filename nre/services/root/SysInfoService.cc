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

#include <services/SysInfo.h>

#include "SysInfoService.h"
#include "Admission.h"
#include "PhysicalMemory.h"
#include "VirtualMemory.h"

using namespace nre;

// text start and end
extern void *_init;
extern void *__fini_array_end;
// data start and end
extern void *__fini_array_end;
extern void *end;

const char *SysInfoService::get_root_info(size_t &virt, size_t &phys, size_t &threads) {
    // find our own module; we're always the first one
    const char *cmdline = "";
    for(auto mem = Hip::get().mem_begin(); mem != Hip::get().mem_end(); ++mem) {
        if(mem->type == HipMem::MB_MODULE) {
            cmdline = mem->cmdline();
            break;
        }
    }
    // determine physical memory by taking the total amount of used mem and substracting the amount
    // we've passed to children
    phys = PhysicalMemory::total_size() - PhysicalMemory::free_size();
    const Child *c;
    for(size_t i = 0; (c = _cm->get_at(i)) != nullptr; ++i) {
        size_t cvirt, cphys;
        c->reglist().memusage(cvirt, cphys);
        phys -= cphys;
    }
    // determine virtual memory by calculating the mem for our text and data area and the one we
    // assign dynamically.
    size_t textsize = reinterpret_cast<uintptr_t>(&__fini_array_end)
                      - reinterpret_cast<uintptr_t>(&_init);
    size_t datasize = reinterpret_cast<uintptr_t>(&end)
                      - reinterpret_cast<uintptr_t>(&__fini_array_end);
    virt = VirtualMemory::used() + textsize + datasize;
    // log and sysinfo
    threads = 2;
    return cmdline;
}

void SysInfoService::portal(capsel_t) {
    UtcbFrameRef uf;
    try {
        SysInfo::Command cmd;
        uf >> cmd;

        switch(cmd) {
            case SysInfo::GET_MEM: {
                uf.finish_input();
                size_t total = PhysicalMemory::total_size();
                size_t free = PhysicalMemory::free_size();
                uf << E_SUCCESS << total << free;
            }
            break;

            case SysInfo::GET_TOTALTIME: {
                cpu_t cpu;
                bool update;
                uf >> cpu >> update;
                uf.finish_input();
                uf << E_SUCCESS << Admission::total_time(cpu, update);
            }
            break;

            case SysInfo::GET_TIMEUSER: {
                size_t idx;
                uf >> idx;
                uf.finish_input();

                String name;
                cpu_t cpu = 0;
                timevalue_t total = 0, time = 0;
                bool res = Admission::get_sched_entity(idx, name, cpu, time, total);
                uf << E_SUCCESS;
                if(res)
                    uf << true << name << cpu << time << total;
                else
                    uf << false;
            }
            break;

            case SysInfo::GET_CHILD: {
                SysInfoService *srv = Thread::current()->get_tls<SysInfoService*>(Thread::TLS_PARAM);
                size_t idx;
                uf >> idx;
                uf.finish_input();

                size_t virt, phys, threads;
                if(idx > 0) {
                    ScopedLock<RCULock> guard(&RCU::lock());
                    idx--;
                    const Child *c = idx >= ChildManager::MAX_CHILDS ? nullptr : srv->_cm->get_at(idx);
                    if(c) {
                        // the main thread is not included in the sc-list
                        threads = c->scs().length() + 1;
                        c->reglist().memusage(virt, phys);

                        uf << E_SUCCESS << true << c->cmdline() << virt << phys << threads;
                    }
                    else
                        uf << E_SUCCESS << false;
                }
                // idx 0 is root
                else {
                    const char *cmdline = srv->get_root_info(virt, phys, threads);
                    uf << E_SUCCESS << true << String(cmdline) << virt << phys << threads;
                }
            }
            break;
        }
    }
    catch(const Exception& e) {
        Syscalls::revoke(uf.delegation_window(), true);
        uf.clear();
        uf << e;
    }
}
