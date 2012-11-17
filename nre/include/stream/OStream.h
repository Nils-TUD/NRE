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

#include <arch/Types.h>
#include <stdarg.h>

namespace nre {

/**
 * The output-stream is used to write formatted output to various destinations. Subclasses have
 * to implement the method to actually write a character. This class provides the higher-level
 * stuff around it.
 */
class OStream {
    /**
     * Collects all formatting parameters from a string
     */
    class FormatParams {
    public:
        enum Flags {
            PADRIGHT    = 1 << 0,
            FORCESIGN   = 1 << 1,
            SPACESIGN   = 1 << 2,
            PRINTBASE   = 1 << 3,
            PADZEROS    = 1 << 4,
            CAPHEX      = 1 << 5,
            LONGLONG    = 1 << 6,
            LONG        = 1 << 7,
            SIZE_T      = 1 << 8,
            INTPTR_T    = 1 << 9,
            POINTER     = 1 << 10,
        };

        explicit FormatParams(const char *fmt, bool read_length, va_list ap);

        const char *end() const {
            return _end;
        }
        uint base() const {
            return _base;
        }
        uint flags() const {
            return _flags;
        }
        uint padding() const {
            return _pad;
        }
        void padding(uint pad) {
            _pad = pad;
        }
        uint precision() const {
            return _prec;
        }
        void precision(uint prec) {
            _prec = prec;
        }

    private:
        uint _base;
        uint _flags;
        uint _pad;
        uint _prec;
        const char *_end;
    };

    /**
     * This class can be written into an OStream to apply formatting while using the stream operators
     */
    template<typename T>
    class Format {
    public:
        explicit Format(const char *fmt, const T &value, uint pad = 0, uint prec = -1)
            : _fmt(fmt), _value(value), _pad(pad), _prec(prec) {
        }

        const char *fmt() const {
            return _fmt;
        }
        const T &value() const {
            return _value;
        }
        uint padding() const {
            return _pad;
        }
        uint precision() const {
            return _prec;
        }

    private:
        const char *_fmt;
        const T &_value;
        uint _pad;
        uint _prec;
    };

    /**
     * We use template specialization to do different formatting operations depending on the
     * type given to fmt().
     */
    template<typename T>
    class FormatImpl {
    };
    template<typename T>
    class FormatImplPtr {
    public:
        void write(OStream &os, const FormatParams &p, T value) {
            os.printptr(reinterpret_cast<uintptr_t>(value), p.flags());
        }
    };
    template<typename T>
    class FormatImplUint {
    public:
        void write(OStream &os, const FormatParams &p, const T &value) {
            // let the user print an integer as a pointer if a wants to. this saves a cast to void*
            if(p.flags() & FormatParams::POINTER)
                os.printptr(value, p.flags());
            // although we rely on the type in most cases, we let the user select between signed
            // and unsigned by specifying certain flags that are only used at one place.
            // this free's the user from having to be really careful whether a value is signed or
            // unsigned, which is especially a problem when using typedefs.
            else if(p.flags() & (FormatParams::FORCESIGN | FormatParams::SPACESIGN))
                os.printnpad(value, p.padding(), p.flags());
            else
                os.printupad(value, p.base(), p.padding(), p.flags());
        }
    };
    template<typename T>
    class FormatImplInt {
    public:
        void write(OStream &os, const FormatParams &p, const T &value) {
            // like above; the base is only used in unsigned print, so do that if the user specified
            // a base (10 is default)
            if(p.base() != 10)
                os.printupad(value, p.base(), p.padding(), p.flags());
            else
                os.printnpad(value, p.padding(), p.flags());
        }
    };
    template<typename T>
    class FormatImplStr {
    public:
        void write(OStream &os, const FormatParams &p, const T &value) {
            if(p.flags() & FormatParams::POINTER)
                os.printptr(reinterpret_cast<uintptr_t>(value), p.flags());
            else
                os.putspad(value, p.padding(), p.precision(), p.flags());
        }
    };

public:
    explicit OStream() {
    }
    virtual ~OStream() {
    }

    template<typename T>
    OStream & operator<<(const Format<T>& fmt) {
        va_list ap{};
        FormatParams p(fmt.fmt(), false, ap);
        p.padding(fmt.padding());
        p.precision(fmt.precision());
        FormatImpl<T>().write(*this, p, fmt.value());
        return *this;
    }
    OStream & operator<<(char c) {
        write(c);
        return *this;
    }
    OStream & operator<<(uchar u) {
        return operator<<(static_cast<ullong>(u));
    }
    OStream & operator<<(short n) {
        return operator<<(static_cast<llong>(n));
    }
    OStream & operator<<(ushort u) {
        return operator<<(static_cast<ullong>(u));
    }
    OStream & operator<<(int n) {
        return operator<<(static_cast<llong>(n));
    }
    OStream & operator<<(uint u) {
        return operator<<(static_cast<ullong>(u));
    }
    OStream & operator<<(long n) {
        return operator<<(static_cast<llong>(n));
    }
    OStream & operator<<(ulong u) {
        return operator<<(static_cast<ullong>(u));
    }
    OStream & operator<<(llong n) {
        printn(n);
        return *this;
    }
    OStream & operator<<(ullong u) {
        printu(u, 10, _hexchars_small);
        return *this;
    }
    OStream & operator<<(const char *str) {
        puts(str, -1);
        return *this;
    }
    OStream & operator<<(const void *p) {
        printptr(reinterpret_cast<uintptr_t>(p), 0);
        return *this;
    }

    void writef(const char *fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        vwritef(fmt, ap);
        va_end(ap);
    }
    void vwritef(const char *fmt, va_list ap);

private:
    virtual void write(char c) = 0;

    void putspad(const char *s, uint pad, uint prec, uint flags);
    void printnpad(llong n, uint pad, uint flags);
    void printupad(ullong u, uint base, uint pad, uint flags);
    int printpad(int count, uint flags);
    int printu(ullong n, uint base, char *chars);
    int printn(llong n);
    void printptr(uintptr_t u, uint flags);
    int puts(const char *str, ulong prec);

    static char _hexchars_big[];
    static char _hexchars_small[];
};

template<>
class OStream::FormatImpl<void*> : public OStream::FormatImplPtr<void*> {
};
template<>
class OStream::FormatImpl<uchar> : public OStream::FormatImplUint<uchar> {
};
template<>
class OStream::FormatImpl<ushort> : public OStream::FormatImplUint<ushort> {
};
template<>
class OStream::FormatImpl<uint> : public OStream::FormatImplUint<uint> {
};
template<>
class OStream::FormatImpl<ulong> : public OStream::FormatImplUint<ulong> {
};
template<>
class OStream::FormatImpl<ullong> : public OStream::FormatImplUint<ullong> {
};
template<>
class OStream::FormatImpl<char> : public OStream::FormatImplInt<char> {
};
template<>
class OStream::FormatImpl<short> : public OStream::FormatImplInt<short> {
};
template<>
class OStream::FormatImpl<int> : public OStream::FormatImplInt<int> {
};
template<>
class OStream::FormatImpl<long> : public OStream::FormatImplInt<long> {
};
template<>
class OStream::FormatImpl<llong> : public OStream::FormatImplInt<llong> {
};
template<>
class OStream::FormatImpl<volatile uint32_t> : public OStream::FormatImplUint<volatile uint32_t> {
};
template<>
class OStream::FormatImpl<const char*> : public OStream::FormatImplStr<const char*> {
};
template<>
class OStream::FormatImpl<char*> : public OStream::FormatImplStr<char*> {
};
// this is necessary to be able to pass a string literal to fmt()
template<int X>
class OStream::FormatImpl<char [X]> : public OStream::FormatImplStr<char [X]> {
};

/**
 * Creates a Format-object that can be written into OStream to write the given value into the
 * stream with specified formatting parameters. This function exists to allow template parameter
 * type inference.
 *
 * Example usage:
 * Serial::get() << "Hello " << fmt(0x1234, "#0x", 8) << "\n";
 *
 * @param value the value to format
 * @param fmt the format parameters
 * @param pad the number of padding characters (default 0)
 * @param precision the precision (default -1 = none)
 * @return the Format object
 */
template<typename T>
static inline OStream::Format<T> fmt(const T &value, const char *fmt, uint pad = 0, uint prec = -1) {
    return OStream::Format<T>(fmt, value, pad, prec);
}
template<typename T>
static inline OStream::Format<T> fmt(const T &value, uint pad = 0, uint prec = -1) {
    return OStream::Format<T>("", value, pad, prec);
}

}
