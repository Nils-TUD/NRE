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
#include <Compiler.h>
#include <stdarg.h>

namespace nre {

/**
 * The output-stream is used to write formatted output to various destinations. Subclasses have
 * to implement the method to actually write a character. This class provides the higher-level
 * stuff around it.
 *
 * It is encouraged to only use the shift operators for writing to the stream. Because writef()
 * is not type-safe, which means you can easily make mistakes and won't notice it. I know that
 * the compiler offers a function attribute to mark printf-like functions so that he can warn
 * us about wrong usages. The problem is that it gets really cumbersome to not produce warnings
 * (so that you actually notice a new warning) when building for multiple architectures. One way
 * to get it right is embedding the length (L, l, ...) into the string via preprocessor defines
 * that depend on the architecture (think of uint64_t). Another way is to add more length-specifier
 * for uintX_t's and similar types. But it gets worse: typedefs. To pass values with typedef-types
 * to writef() in a correct way you would have to look at the "implementation" of the typedef, i.e.
 * the type it is mapped to. This makes it really hard to change typedefs afterwards. So, to avoid
 * that you would have to introduce a length modifier for each typedef in your code. Not only that
 * you'll run out of ASCII chars, this is simply not acceptable.
 *
 * Therefore we go a different way here. We try to use only shift operators and provide a quite
 * concise and simple way to add formatting parameters (in contrast to the really verbose concept
 * of the standard C++ iostreams). This way we have type-safety (you can't accidently output a
 * string as an integer or the other way around) and it's still convenient enough to pass formatting
 * parameters. This is done via template specialization. We provide a freestanding function named
 * fmt() to make use of template parameter type inference which receives all necessary information,
 * wraps them into an object and writes this object into the stream. The method for that will create
 * the corresponding template class, depending on the type to print, which in turn will finally
 * print the value.
 * The whole formatting stuff is done by OStream::FormatParams. It receives a string with formatting
 * arguments (same syntax as printf) and whether the printf-behaviour is desired or a limited one.
 * For the fmt() function we use the limited one which does not support padding and precision and
 * recognizes only the base instead of the type (i.e. only x, o, and so on and not s, c, ...). The
 * padding and precision are always passed in a separate parameter to allow "dynamic" values and
 * only have one place to specify them.
 * There is still one problem: the difference between signed and unsigned values tends to be ignored
 * by programmers. That is, I guess most people will assume that fmt(0x1234, "x") prints it with
 * a hexadecimal base. Strictly speaking, this is wrong, because 0x1234 is an int, not unsigned int,
 * and only unsigned integers are printed in a base different than 10. Thus, one would have to use
 * fmt(0x1234U, "x") to achieve it. This is even worse for typedefs or when not passing a literal,
 * because you might not even know whether its signed or unsigned. To prevent that problem I've
 * decided to interpret some formatting parameters as hints. Since the base will only be considered
 * for unsigned values, fmt(0x1234, "x") will print 0x1234 as an unsigned integer in base 16.
 * Similarly, when passing "+" or " " it will be printed as signed, even when its unsigned. And
 * finally, "p" forces a print as a pointer (xxxx:xxxx) even when its some unsigned type or char*.
 *
 * The syntax of writef() and fmt() is the same with the only difference that in fmt() it does not
 * start with '%' (there is exactly one thing to format anyway) and has the above mentioned
 * limitations. Thus it is described together here.
 *
 * The basic syntax is: [flags][padding][.precision][length][type]
 * Where [flags] is any combination of:
 * - '-': add padding on the right side instead of on the left
 * - '+': always print the sign (+ or -); forces a signed print
 * - ' ': print a space in front of positive values; forces a signed print
 * - '#': print the base
 * - '0': use zeros for padding instead of spaces
 *
 * [padding] and [.precision] is ignored in fmt(). It can be either '*' which expects the number of
 * characters as an argument. Or [0-9]+. Precision expects a '.' before that.
 *
 * [length] is ignored in fmt(). It can be:
 * - 'l': for long
 * - 'L': for long long
 * - 'z': for size_t
 * - 'P': for intptr_t
 *
 * [type] can be:
 * - 'd', 'i':      a signed integer; ignored in fmt()
 * - 'p':           a pointer; forces a pointer print
 * - 'u':           an unsigned integer, printed in base 10; ignored in fmt()
 * - 'b':           an unsigned integer, printed in base 2; forces an unsigned print
 * - 'o':           an unsigned integer, printed in base 8; forces an unsigned print
 * - 'x':           an unsigned integer, printed in base 16; forces an unsigned print
 * - 'X':           an unsigned integer, printed in base 16 with capital letters; forces an
 *                  unsigned print
 * - 's':           a string; ignored in fmt()
 * - 'c':           a character; ignored in fmt()
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

        explicit FormatParams(const char *fmt, bool all, va_list *ap);

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

    /**
     * Writes a value into the stream with formatting applied. This operator should not be used
     * directly, but fmt() should be used instead (because its shorter).
     */
    template<typename T>
    OStream & operator<<(const Format<T>& fmt) {
        va_list ap{};
        FormatParams p(fmt.fmt(), false, &ap);
        p.padding(fmt.padding());
        p.precision(fmt.precision());
        FormatImpl<T>().write(*this, p, fmt.value());
        return *this;
    }

    /**
     * Writes the given character/integer into the stream, without formatting
     */
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

    /**
     * Writes the given string into the stream
     */
    OStream & operator<<(const char *str) {
        puts(str, -1);
        return *this;
    }
    /**
     * Writes the given pointer into the stream (xxxx:xxxx)
     */
    OStream & operator<<(const void *p) {
        printptr(reinterpret_cast<uintptr_t>(p), 0);
        return *this;
    }

    /**
     * Puts the given arguments formatted according to <fmt> into the stream.
     * The use of this function is STRONGLY DISCOURAGED. Please use the stream operators with
     * fmt() to apply formatting parameters. See the comment of OStream for further explanations.
     * To give you at least the illusion of safety, FMT_PRINTF is used to warn you about errors ;)
     *
     * @param fmt the formatting specification
     */
    FMT_PRINTF(2, 3) void writef(const char *fmt, ...) {
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
// unfortunatly, we have to add special templates for volatile :( uint32_t is enough for now
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
