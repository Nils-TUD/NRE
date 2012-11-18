/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2010, Bernhard Kauer <bk@vmmon.org>
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

#include <services/PCIConfig.h>

#include "HostACPI.h"

/**
 * ATARE - ACPI table IRQ routing extraction.
 *
 * This extends the ideas from "ATARE: ACPI Tables and Regular
 * Expressions, Bernhard Kauer, TU Dresden technical report
 * TUD-FI09-09, Dresden, Germany, August 2009".
 *
 * State: testing
 * Features: direct PRT, referenced PRTs, exact name resolution, Routing Entries
 */
class HostATARE {
    /**
     * A single PCI routing entry.
     */
    struct PciRoutingEntry {
        PciRoutingEntry * next;
        unsigned adr;
        unsigned char pin;
        unsigned char gsi;
        PciRoutingEntry(PciRoutingEntry *_next, unsigned _adr, unsigned char _pin, unsigned char _gsi)
            : next(_next), adr(_adr), pin(_pin), gsi(_gsi) {
        }
    };

    /*
     * A reference to a named entry.
     * Name is an absolute name in the ACPI namespace without the leading backslash.
     */
    struct NamedRef {
        NamedRef *next;
        const uint8_t *ptr;
        size_t len;
        const char *name;
        size_t namelen;
        PciRoutingEntry *routing;
        NamedRef(NamedRef *_next, const uint8_t *_ptr, size_t _len, const char *_name, size_t _namelen)
            : next(_next), ptr(_ptr), len(_len), name(_name), namelen(_namelen), routing(nullptr) {
        }
    };

public:
    typedef nre::PCIConfig::value_type value_type;

    explicit HostATARE(HostACPI &acpi, uint debug) : _head(nullptr) {
        // add entries from the SSDT
        const HostACPI::ACPIListItem *item = acpi.find("DSDT", 0);
        if(item)
            _head = add_refs(reinterpret_cast<uint8_t*>(item->start()), item->length(), _head);

        // and from the SSDTs
        for(uint i = 0; (item = acpi.find("SSDT", i)) != 0; ++i)
            _head = add_refs(reinterpret_cast<uint8_t*>(item->start()), item->length(), _head);

        add_routing(_head);
        if(debug & 1)
            debug_show_items();
        if(debug & 2)
            debug_show_routing();
    }

    uint get_gsi(nre::BDF bdf, uint8_t pin, nre::BDF parent_bdf) {
        // find the device
        for(NamedRef *dev = _head; dev; dev = dev->next) {
            if(dev->ptr[0] == 0x82 && get_device_bdf(_head, dev) == parent_bdf) {
                // look for the right entry
                for(PciRoutingEntry *p = dev->routing; p; p = p->next) {
                    if((p->adr >> 16) == bdf.device() && (pin == p->pin))
                        return p->gsi;
                }
            }
        }
        VTHROW(Exception, E_NOT_FOUND,
               "ATARE was unable to find GSI for " << bdf << " parent " << parent_bdf << " pin " << pin);
    }

private:
    void debug_show_items();
    void debug_show_routing();

    static size_t get_pkgsize_len(const uint8_t *data);
    static size_t read_pkgsize(const uint8_t *data);
    static bool name_seg(const uint8_t *res);
    static size_t get_nameprefix_len(const uint8_t *table);
    static size_t get_name_len(const uint8_t *table);
    static void get_absname(NamedRef *parent, const uint8_t *name, size_t &namelen, char *res,
                            size_t skip = 0);
    static uint read_data(const uint8_t *data, size_t &length);
    static ssize_t get_packet4(const uint8_t *table, ssize_t len, uint *x);
    static void search_prt_direct(NamedRef *dev, NamedRef *ref);
    static void search_prt_indirect(NamedRef *head, NamedRef *dev, NamedRef *prt);
    static uint get_namedef_value(NamedRef *head, NamedRef *parent, const char *name);
    static NamedRef *search_ref(NamedRef *head, NamedRef *parent, const char *name, bool upstream);
    static nre::BDF get_device_bdf(NamedRef *head, NamedRef *dev);
    static NamedRef *add_refs(const uint8_t *table, unsigned len, NamedRef *res = nullptr);
    static void add_routing(NamedRef *head);

    NamedRef *_head;
};
