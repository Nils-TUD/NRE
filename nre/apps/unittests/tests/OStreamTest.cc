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

#include <stream/OStringStream.h>
#include <util/Float.h>

#include "OStreamTest.h"

using namespace nre;
using namespace nre::test;

static void test_writef();
static void test_stream_ops();

const TestCase ostream_writef = {
    "OStream writef", test_writef
};
const TestCase ostream_strops = {
    "OStream stream operators", test_stream_ops
};

#define STREAM_CHECK(expr, expstr) do {                                                     \
        OStringStream __os(str, sizeof(str));                                               \
        __os << expr;                                                                       \
        WRITEF_CHECK(-1, -1, str, expstr);                                                  \
    } while(0)

#define WRITEF_CHECK(res, expres, recvstr, expstr) do {                                     \
        WVPASSEQ(recvstr, expstr);                                                          \
        if((res) != -1 && (expres) == -1) {                                                 \
            WVPASSEQ(res, (int)strlen(expstr));                                             \
        }                                                                                   \
        else if((res) != -1) {                                                              \
            WVPASSEQ(res, expres);                                                          \
        }                                                                                   \
    } while(0)

static void test_writef() {
    char str[200];
    int res;

    res = OStringStream(str, sizeof(str)).writef("%d", 4);
    WRITEF_CHECK(res, -1, str, "4");

    res = OStringStream(str, sizeof(str)).writef("%i%d", 123, 456);
    WRITEF_CHECK(res, -1, str, "123456");

    res = OStringStream(str, sizeof(str)).writef("_%d_%d_", 1, 2);
    WRITEF_CHECK(res, -1, str, "_1_2_");

    res = OStringStream(str, sizeof(str))
            .writef("x=%x, X=%X, o=%o, d=%d, u=%u", 0xABC, 0xDEF, 0723, -675, 412);
    WRITEF_CHECK(res, -1, str, "x=abc, X=DEF, o=723, d=-675, u=412");

    res = OStringStream(str, sizeof(str)).writef("'%s'_%c_", "test", 'f');
    WRITEF_CHECK(res, -1, str, "'test'_f_");

    res = OStringStream(str, sizeof(str)).writef("%s", "");
    WRITEF_CHECK(res, -1, str, "");

    res = OStringStream(str, sizeof(str)).writef("%10s", "padme");
    WRITEF_CHECK(res, -1, str, "     padme");

    res = OStringStream(str, sizeof(str)).writef("%02d, %04x", 9, 0xff);
    WRITEF_CHECK(res, -1, str, "09, 00ff");

    res = OStringStream(str, sizeof(str))
            .writef("%p, %x", reinterpret_cast<void*>(0xdeadbeef), 0x12345678);
    if(sizeof(uintptr_t) == 4)
        WRITEF_CHECK(res, -1, str, "dead:beef, 12345678");
    else if(sizeof(uintptr_t) == 8)
        WRITEF_CHECK(res, -1, str, "0000:0000:dead:beef, 12345678");
    else
        WVFAIL(true);

    res = OStringStream(str, sizeof(str))
            .writef("%Ld, %017Ld, %-*Ld", 1LL, 8167127123123123LL, 12, -81273123LL);
    WRITEF_CHECK(res, -1, str, "1, 08167127123123123, -81273123   ");

    res = OStringStream(str, sizeof(str))
            .writef("%Lu, %017Lx, %#-*LX", 1ULL, 0x7179bafed2122ULL, 12, 0x1234ABULL);
    WRITEF_CHECK(res, -1, str, "1, 00007179bafed2122, 0X1234AB    ");

    res = OStringStream(str, sizeof(str))
            .writef("%f, %f, %f, %f, %f, %f", 0.f, 1.f, -1.f, 0.f, 0.4f, 18.4f);
    WRITEF_CHECK(res, -1, str, "0.000000, 1.000000, -1.000000, 0.000000, 0.400000, 18.399999");

    res = OStringStream(str, sizeof(str))
            .writef("%f, %f, %f, %f", -1.231f, 999.999f, 1234.5678f, 1189378123.78167213123f);
    WRITEF_CHECK(res, -1, str, "-1.230999, 999.999023, 1234.567749, 1189378176.000000");

    res = OStringStream(str, sizeof(str))
            .writef("%lf, %lf, %lf, %lf, %lf, %lf", 0., 1., -1., 0., 0.4, 18.4);
    WRITEF_CHECK(res, -1, str, "0.000000, 1.000000, -1.000000, 0.000000, 0.400000, 18.399999");

    res = OStringStream(str, sizeof(str))
            .writef("%lf, %lf, %lf, %lf", -1.231, 999.999, 1234.5678, 1189378123.78167213123);
    WRITEF_CHECK(res, -1, str, "-1.231000, 999.999000, 1234.567800, 1189378123.781672");

    res = OStringStream(str, sizeof(str))
            .writef("%8.4lf, %8.1lf, %8.10lf", 1., -1., 0.);
    WRITEF_CHECK(res, -1, str, "  1.0000,     -1.0, 0.0000000000");

    res = OStringStream(str, sizeof(str))
            .writef("%3.0lf, %-6.1lf, %2.4lf, %10.10lf",
                    -1.231, 999.999, 1234.5678, 1189378123.78167213123);
    WRITEF_CHECK(res, -1, str, "-1., 999.9 , 1234.5678, 1189378123.7816722393");
}

static void test_stream_ops() {
    char str[200];

    STREAM_CHECK(123 << 456, "123456");
    STREAM_CHECK("_" << 1 << "_" << 2 << "_", "_1_2_");
    STREAM_CHECK("" << "x=" << fmt(0xABC, "x") << ", "
                    << "X=" << fmt(0xDEF, "X") << ", "
                    << "o=" << fmt(0723, "o") << ", "
                    << "d=" << -675 << ", "
                    << "u=" << 412,
                 "x=abc, X=DEF, o=723, d=-675, u=412");
    STREAM_CHECK("_'" << "test" << "'_" << 'f' << "_", "_'test'_f_");
    STREAM_CHECK(fmt("padme", 10), "     padme");
    STREAM_CHECK(fmt(9, "0", 2) << ", " << fmt(0xff, "0x", 4), "09, 00ff");

    OStringStream os(str, sizeof(str));
    os << fmt(0xdeadbeef, "p") << ", " << fmt(0x12345678, "x");
    if(sizeof(uintptr_t) == 4)
        WRITEF_CHECK(-1, -1, str, "dead:beef, 12345678");
    else if(sizeof(uintptr_t) == 8)
        WRITEF_CHECK(-1, -1, str, "0000:0000:dead:beef, 12345678");
    else
        WVFAIL(true);

    STREAM_CHECK(1LL << ", " << fmt(8167127123123123LL, "0", 17)
                     << ", " << fmt(-81273123LL, "-", 12),
                 "1, 08167127123123123, -81273123   ");
    STREAM_CHECK(1ULL << ", " << fmt(0x7179bafed2122ULL, "0x", 17)
                      << ", " << fmt(0x1234ABULL, "#-X", 12),
                 "1, 00007179bafed2122, 0X1234AB    ");

    STREAM_CHECK(0.f << ", " << 1.f << ", " << -1.f << ", " << 0.f << ", " << 0.4f << ", " << 18.4f,
                 "0.000000, 1.000000, -1.000000, 0.000000, 0.400000, 18.399999");
    STREAM_CHECK(-1.231f << ", " << 999.999f << ", " << 1234.5678f << ", " << 1189378123.78167213123f,
                 "-1.230999, 999.999023, 1234.567749, 1189378176.000000");
    STREAM_CHECK(fmt(1., 8, 4) << ", " << fmt(-1., 8, 1) << ", " << fmt(0., 8, 10),
                 "  1.0000,     -1.0, 0.0000000000");
    STREAM_CHECK(fmt(-1.231, 3, 0) << ", " << fmt(999.999, "-", 6, 1)
                                   << ", " << fmt(1234.5678, 2, 4)
                                   << ", " << fmt(1189378123.78167213123, 10, 10),
                 "-1., 999.9 , 1234.5678, 1189378123.7816722393");

    STREAM_CHECK(Float<float>::inf() << ", " << -Float<float>::inf(), "inf, -inf");
    STREAM_CHECK(Float<float>::nan() << ", " << -Float<float>::nan(), "nan, -nan");
    STREAM_CHECK(Float<double>::inf() << ", " << -Float<double>::inf(), "inf, -inf");
    STREAM_CHECK(Float<double>::nan() << ", " << -Float<double>::nan(), "nan, -nan");
}
