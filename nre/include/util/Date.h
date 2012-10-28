/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2009, Bernhard Kauer <bk@vmmon.org>
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

#include <util/Math.h>

namespace nre {

struct DateInfo {
    int sec;
    int min;
    int hour;
    int mday;
    int mon;
    int year;
    int wday;
    int yday;
    int isdsdt;

    explicit DateInfo() : sec(), min(), hour(), mday(), mon(), year(), wday(), yday(), isdsdt() {
    }
    explicit DateInfo(int _year, int _mon, int _mday, int _hour, int _min, int _sec)
        : sec(_sec), min(_min), hour(_hour), mday(_mday), mon(_mon), year(_year), wday(0), yday(0),
          isdsdt(0) {
    }
};

class Date {
public:
    static timevalue_t mktime(const DateInfo *tm) {
        bool before_march = tm->mon < 3;
        timevalue_t days = ((tm->mon + 10) * 367) / 12 + 2 * before_march - 719866
                           + tm->mday - before_march * is_leap(tm->year) + tm->year * 365;
        days += tm->year / 4 - tm->year / 100 + tm->year / 400;
        return ((days * 24 + tm->hour) * 60 + tm->min) * 60 + tm->sec;
    }

    static void gmtime(timevalue_t seconds, DateInfo *tm) {
        static uint leap[] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366};
        static uint noleap[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
        // move from 1970 to 1 as epoch, to be able to use division with positive values
        seconds = seconds + (719528ull - 366ull) * 86400ull;
        tm->sec = Math::moddiv<timevalue_t>(seconds, 60);
        tm->min = Math::moddiv<timevalue_t>(seconds, 60);
        tm->hour = Math::moddiv<timevalue_t>(seconds, 24);
        timevalue_t days = seconds++;
        tm->wday = Math::moddiv<timevalue_t>(seconds, 7);
        uint years400 = Math::divmod<timevalue_t>(days, 4 * 36524 + 1);
        uint years100 = Math::divmod<timevalue_t>(days, 36524);
        // overflow on a 400year boundary?
        if(years100 == 4) {
            years100--;
            days += 36524;
        }

        uint years4 = Math::divmod<timevalue_t>(days, 1461);
        uint years = Math::divmod<timevalue_t>(days, 365);
        // overflow on the 4year boundary?
        if(years == 4) {
            years -= 1;
            days += 365;
        }

        // move back to our timebase
        tm->year = ((((years400 << 2) + years100) * 25 + years4) << 2) + years + 1;
        tm->yday = days + 1;

        // get month,day from day_of_year
        uint *l = is_leap(tm->year) ? leap : noleap;

        // heuristic to (under-)estimate the month
        uint m = static_cast<uint>(days) / 31;
        days -= l[m];
        // did we made it?
        if(days >= (l[m + 1] - l[m])) {
            days -= l[m + 1] - l[m];
            m += 1;
        }
        tm->mon = m + 1;
        tm->mday = days + 1;
    }

private:
    static bool is_leap(int year) {
        return (!(year & 3) && ((year % 100) != 0)) || (year % 400 == 0);
    }

    Date();
};

}
