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

#include <stream/OStream.h>
#include <util/Digits.h>
#include <cstring>

namespace nre {

char OStream::_hexchars_big[]     = "0123456789ABCDEF";
char OStream::_hexchars_small[]   = "0123456789abcdef";

OStream::FormatParams::FormatParams(const char *fmt, bool all, va_list *ap)
        : _base(10), _flags(0), _pad(0), _prec(-1), _end() {
    // read flags
    bool readFlags = true;
    while(readFlags) {
        switch(*fmt) {
            case '-':
                _flags |= PADRIGHT;
                fmt++;
                break;
            case '+':
                _flags |= FORCESIGN;
                fmt++;
                break;
            case ' ':
                _flags |= SPACESIGN;
                fmt++;
                break;
            case '#':
                _flags |= PRINTBASE;
                fmt++;
                break;
            case '0':
                _flags |= PADZEROS;
                fmt++;
                break;
            default:
                readFlags = false;
                break;
        }
    }

    if(all) {
        // read pad-width
        if(*fmt == '*') {
            _pad = va_arg(*ap, ulong);
            fmt++;
        }
        else {
            while(*fmt >= '0' && *fmt <= '9') {
                _pad = _pad * 10 + (*fmt - '0');
                fmt++;
            }
        }

        // read precision
        if(*fmt == '.') {
            if(*++fmt == '*') {
                _prec = va_arg(*ap, ulong);
                fmt++;
            }
            else {
                _prec = 0;
                while(*fmt >= '0' && *fmt <= '9') {
                    _prec = _prec * 10 + (*fmt - '0');
                    fmt++;
                }
            }
        }

        // read length
        switch(*fmt) {
            case 'l':
                _flags |= FormatParams::LONG;
                fmt++;
                break;
            case 'L':
                _flags |= LONGLONG;
                fmt++;
                break;
            case 'z':
                _flags |= SIZE_T;
                fmt++;
                break;
            case 'P':
                _flags |= INTPTR_T;
                fmt++;
                break;
        }
    }

    // read base
    switch(*fmt) {
        case 'X':
        case 'x':
            if(*fmt == 'X')
                _flags |= CAPHEX;
            _base = 16;
            break;
        case 'o':
            _base = 8;
            break;
        case 'b':
            _base = 2;
            break;
        case 'p':
            _flags |= POINTER;
            break;
    }
    _end = fmt;
}

void OStream::vwritef(const char *fmt, va_list ap0) {
    // depending on the implementation of va_list, we might not be able to get a pointer to a
    // va_list that has been passed in as a function parameter. as a workaround we use va_copy
    // to copy the arguments to a local va_list, so that we can pass a pointer to that to a
    // function.
    va_list ap;
    va_copy(ap, ap0);
    while(1) {
        char c;
        // wait for a '%'
        while((c = *fmt++) != '%') {
            write(c);
            // finished?
            if(c == '\0')
                return;
        }

        // read format parameter
        FormatParams p(fmt, true, &ap);
        fmt = p.end();

        switch(c = *fmt++) {
            // signed integer
            case 'd':
            case 'i':
                llong n;
                if(p.flags() & FormatParams::LONGLONG)
                    n = va_arg(ap, llong);
                else if(p.flags() & FormatParams::LONG)
                    n = va_arg(ap, long);
                else if(p.flags() & FormatParams::SIZE_T)
                    n = va_arg(ap, ssize_t);
                else if(p.flags() & FormatParams::INTPTR_T)
                    n = va_arg(ap, intptr_t);
                else
                    n = va_arg(ap, int);
                printnpad(n, p.padding(), p.flags());
                break;

            // pointer
            case 'p': {
                uintptr_t u = va_arg(ap, uintptr_t);
                printptr(u, p.flags());
                break;
            }

            // unsigned integer
            case 'b':
            case 'u':
            case 'o':
            case 'x':
            case 'X': {
                ullong u;
                if(p.flags() & FormatParams::LONGLONG)
                    u = va_arg(ap, ullong);
                else if(p.flags() & FormatParams::LONG)
                    u = va_arg(ap, ulong);
                else if(p.flags() & FormatParams::SIZE_T)
                    u = va_arg(ap, size_t);
                else if(p.flags() & FormatParams::INTPTR_T)
                    u = va_arg(ap, uintptr_t);
                else
                    u = va_arg(ap, uint);
                printupad(u, p.base(), p.padding(), p.flags());
                break;
            }

            // string
            case 's': {
                char *s = va_arg(ap, char*);
                putspad(s, p.padding(), p.precision(), p.flags());
                break;
            }

            // character
            case 'c': {
                char b = va_arg(ap, uint);
                write(b);
                break;
            }

            default:
                write(c);
                break;
        }
    }
}

void OStream::putspad(const char *s, uint pad, uint prec, uint flags) {
    if(pad > 0 && !(flags & FormatParams::PADRIGHT)) {
        ulong width = prec != static_cast<uint>(-1) ? prec : strlen(s);
        printpad(pad - width, flags);
    }
    int n = puts(s, prec);
    if(pad > 0 && (flags & FormatParams::PADRIGHT))
        printpad(pad - n, flags);
}

void OStream::printnpad(llong n, uint pad, uint flags) {
    int count = 0;
    // pad left
    if(!(flags & FormatParams::PADRIGHT) && pad > 0) {
        size_t width = Digits::count_signed(n, 10);
        if(n > 0 && (flags & (FormatParams::FORCESIGN | FormatParams::SPACESIGN)))
            width++;
        count += printpad(pad - width, flags);
    }
    // print '+' or ' ' instead of '-'
    if(n > 0) {
        if((flags & FormatParams::FORCESIGN)) {
            write('+');
            count++;
        }
        else if(((flags) & FormatParams::SPACESIGN)) {
            write(' ');
            count++;
        }
    }
    // print number
    count += printn(n);
    // pad right
    if((flags & FormatParams::PADRIGHT) && pad > 0)
        printpad(pad - count, flags);
}

void OStream::printupad(ullong u, uint base, uint pad, uint flags) {
    int count = 0;
    // pad left - spaces
    if(!(flags & FormatParams::PADRIGHT) && !(flags & FormatParams::PADZEROS) && pad > 0) {
        size_t width = Digits::count_unsigned(u, base);
        count += printpad(pad - width, flags);
    }
    // print base-prefix
    if((flags & FormatParams::PRINTBASE)) {
        if(base == 16 || base == 8) {
            write('0');
            count++;
        }
        if(base == 16) {
            char c = (flags & FormatParams::CAPHEX) ? 'X' : 'x';
            write(c);
            count++;
        }
    }
    // pad left - zeros
    if(!(flags & FormatParams::PADRIGHT) && (flags & FormatParams::PADZEROS) && pad > 0) {
        size_t width = Digits::count_unsigned(u, base);
        count += printpad(pad - width, flags);
    }
    // print number
    if(flags & FormatParams::CAPHEX)
        count += printu(u, base, _hexchars_big);
    else
        count += printu(u, base, _hexchars_small);
    // pad right
    if((flags & FormatParams::PADRIGHT) && pad > 0)
        printpad(pad - count, flags);
}

int OStream::printpad(int count, uint flags) {
    int res = count;
    char c = flags & FormatParams::PADZEROS ? '0' : ' ';
    while(count-- > 0)
        write(c);
    return res;
}

int OStream::printu(ullong n, uint base, char *chars) {
    int res = 0;
    if(n >= base)
        res += printu(n / base, base, chars);
    write(chars[(n % base)]);
    return res + 1;
}

int OStream::printn(llong n) {
    int res = 0;
    if(n < 0) {
        write('-');
        n = -n;
        res++;
    }

    if(n >= 10)
        res += printn(n / 10);
    write('0' + n % 10);
    return res + 1;
}

void OStream::printptr(uintptr_t u, uint flags) {
    size_t size = sizeof(uintptr_t);
    flags |= FormatParams::PADZEROS;
    // 2 hex-digits per byte and a ':' every 2 bytes
    while(size > 0) {
        printupad((u >> (size * 8 - 16)) & 0xFFFF, 16, 4, flags);
        size -= 2;
        if(size > 0)
            write(':');
    }
}

int OStream::puts(const char *str, ulong prec) {
    const char *begin = str;
    char c;
    while((prec == static_cast<ulong>(-1) || prec-- > 0) && (c = *str)) {
        write(c);
        str++;
    }
    return str - begin;
}

}
