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

#include <utcb/UtcbBase.h>
#include <util/Math.h>
#include <Assert.h>
#include <cstring>

namespace nre {

class UtcbFrameRef;
class UtcbFrame;

/**
 * The copying UTCB implementation. That is, it creates frames in the UTCB by copying the current
 * frame content to the middle of the UTCB. This is of course slower than the stack-based
 * implementation, but requires no modification of NOVA.
 */
class Utcb : public UtcbBase {
    friend class UtcbFrameRef;
    friend class UtcbFrame;

    static const size_t POS_SLOT    = ((SIZE - sizeof(UtcbHead)) / sizeof(word_t)) / 2;
    static const size_t SLOTS       = WORDS - sizeof(UtcbHead) / sizeof(word_t);

    /**
     * @param frame the current frame
     * @return the pointer to the start of the typed items
     */
    static word_t *get_top(Utcb *frame) {
        size_t utcbtop = Math::round_dn<size_t>(reinterpret_cast<size_t>(frame + 1), Utcb::SIZE);
        return reinterpret_cast<word_t*>(utcbtop);
    }
    static word_t *get_top(Utcb *frame, size_t) {
        return get_top(frame);
    }
    /**
     * @param base the UTCB base
     * @return the current frame (always base here since we always start at the bottom)
     */
    static Utcb *get_current_frame(Utcb *base) {
        return base;
    }

    /**
     * @return current layer
     */
    uint layer() const {
        return (msg[POS_SLOT] >> 12) & 0xF;
    }
    /**
     * @return number of untyped items including the saved ones
     */
    uint untyped_count() const {
        return msg[POS_SLOT] & 0xFFF;
    }
    /**
     * @return the offset in the message-registers where new untyped items should be saved
     */
    uint untyped_start() const {
        return POS_SLOT - untyped_count();
    }
    /**
     * Adds or substracts <n> from the untyped items
     */
    void add_untyped(int n) {
        short ut = untyped_count();
        msg[POS_SLOT] = (msg[POS_SLOT] & 0xFFFFF000) | (ut + n);
    }
    /**
     * @return number of typed items including the saved ones
     */
    uint typed_count() const {
        return msg[POS_SLOT] >> 16;
    }
    /**
     * @return the offset in the message-registers where new typed items should be saved
     */
    uint typed_start() const {
        return POS_SLOT + 1 + typed_count();
    }
    /**
     * Adds or substracts <n> from the typed items
     */
    void add_typed(int n) {
        short t = typed_count();
        msg[POS_SLOT] = (msg[POS_SLOT] & 0x0000FFFF) | (t + n) << 16;
    }
    /**
     * Sets the layer to <layer>
     */
    void set_layer(int layer) {
        assert(layer >= 0 && layer <= 0xF);
        msg[POS_SLOT] = (msg[POS_SLOT] & 0xFFFF0FFF) | (layer << 12);
    }

    /**
     * @return the pointer to the UTCB base, i.e. the first frame
     */
    Utcb *base() const {
        return reinterpret_cast<Utcb*>(reinterpret_cast<uintptr_t>(this) & ~(SIZE - 1));
    }
    /**
     * @return the number of free typed items (i.e. the number of typed items you can still add)
     */
    size_t free_typed() const {
        return (SLOTS - POS_SLOT - 1 - typed_count() - typed * 2) / 2;
    }
    /**
     * @return the number of free untyped items
     */
    size_t free_untyped() const {
        return POS_SLOT - untyped_count() - untyped;
    }

    /**
     * Creates a new layer. Every instance of UtcbFrameRef or UtcbFrame creates a layer. This way,
     * we know whether someone currently uses the UTCB.
     */
    void push_layer() {
        set_layer(layer() + 1);
    }
    /**
     * Removes the topmost layer
     */
    void pop_layer() {
        set_layer(layer() - 1);
    }

    /**
     * Creates a new frame in the UTCB. If necessary, it saves the current UTCB content by copying
     * it to the corresponding place in the middle.
     */
    Utcb *push(word_t *&) {
        uint l = layer();
        if(l > 1) {
            const int utcount = (sizeof(UtcbHead) / sizeof(word_t)) + untyped;
            const int tcount = typed * 2;
            word_t *utbackup = msg + untyped_start();
            word_t *tbackup = msg + typed_start();
            memcpy(utbackup - utcount, this, utcount * sizeof(word_t));
            utbackup[-(utcount + 1)] = utcount;
            memcpy(tbackup, get_top(this) - tcount, tcount * sizeof(word_t));
            tbackup[tcount] = tcount;
            add_untyped(utcount + 1);
            add_typed(tcount + 1);
        }
        reset();
        return this;
    }
    /**
     * Removes the topmost frame from the UTCB. That is, if necessary, it restores the old content
     * from the corresponding place in the middle.
     */
    void pop() {
        uint l = layer();
        if(l > 1) {
            const word_t *utbackup = msg + untyped_start();
            const word_t *tbackup = msg + typed_start();
            const int utcount = utbackup[0];
            const int tcount = tbackup[-1];
            memcpy(this, utbackup + 1, utcount * sizeof(word_t));
            memcpy(get_top(this) - tcount, tbackup - tcount - 1, tcount * sizeof(word_t));
            add_untyped(-(utcount + 1));
            add_typed(-(tcount + 1));
        }
    }
};

}
