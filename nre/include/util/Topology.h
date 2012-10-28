/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2007-2011, Bernhard Kauer <bk@vmmon.org>
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

#include <arch/Types.h>
#include <util/QuickSort.h>
#include <CPU.h>

namespace nre {

/**
 * This class allows to divide given CPUs according the distance on the chip/board.
 */
class Topology {
    struct SortCPU {
        cpu_t id;
        uint8_t thread;
        uint8_t core;
        uint8_t package;
    };

public:
    /**
     * Takes the CPUs from <begin>..<end> and splits them in <part> paritions, depending on their
     * distance. Afterwards you have two arrays:
     * 1. part_cpu which maps from a parition to a CPU id
     * 2. cpu_cpu which maps from a CPU to a CPU nearby
     *
     * @param begin the beginning of the CPUs
     * @param end the end of the CPUs
     * @param n
     *  IN: Number of Hip_cpu descriptors
     *  OUT: Number of CPUs in mappings
     * @param parts
     *  IN: Desired parts
     *  OUT: Actual parts (<= n)
     * @param part_cpu Part -> responsible CPU mapping
     * @param cpu_cpu CPU -> responsible CPU mapping
     */
    static void divide(const CPU::iterator begin, const CPU::iterator end, size_t &n, size_t &parts,
                       cpu_t part_cpu[], cpu_t cpu_cpu[]) {
        // Copy CPU descriptors into consecutive list.
        SortCPU local[n];
        size_t i = 0;
        for(CPU::iterator it = begin; it != end; ++it, ++i) {
            local[i].id = it->log_id();
            local[i].thread = it->thread();
            local[i].core = it->core();
            local[i].package = it->package();
        }
        if(i < parts)
            parts = i;
        n = i;

        Quicksort<SortCPU>::sort(cmp_cpus, local, n);

        // Divide list into parts. Update mappings.
        uint cpus_per_part = n / parts;
        uint cpus_rest = n % parts;
        cpu_t cur_cpu = 0;
        for(size_t i = 0; i < parts; i++) {
            uint cpus_to_spend = cpus_per_part;
            if(cpus_rest) {
                cpus_to_spend++;
                cpus_rest--;
            }
            part_cpu[i] = local[cur_cpu].id;
            for(uint j = 0; j < cpus_to_spend; j++, cur_cpu++)
                cpu_cpu[local[cur_cpu].id] = part_cpu[i];
        }
    }

private:
    static bool cmp_cpus(const SortCPU &a, const SortCPU &b) {
        uint a_v = (a.package << 16) | (a.core << 8) | (a.thread);
        uint b_v = (b.package << 16) | (b.core << 8) | (b.thread);
        return (a_v <= b_v);
    }

    Topology();
};

}
