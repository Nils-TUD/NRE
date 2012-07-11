/*
 * TODO comment me
 *
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NUL.
 *
 * NUL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NUL is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <kobj/Ports.h>
#include <util/Clock.h>
#include <util/Date.h>
#include <Assert.h>

class HostRTC {
	enum {
		PORT_BASE	= 0x70,
	    MS_TIMEOUT	= 2000, // milliseconds
	};

public:
	explicit HostRTC() : _ports(PORT_BASE,2), _clock(1000) {
	}

	/**
	 * Waits for the next RTC update
	 */
	void sync() {
		// ensure that the RTC counts
		assert(!(read(0xb) & 0x80));

		/*
		 * We wait for an update to happen to get a more accurate timing.
		 *
		 * Instead of triggering on the UIP flag, which is typically
		 * enabled for less than 2ms, we wait on the Update-Ended-Flag
		 * which is set on the falling edge of the UIP flag and sticky
		 * until read out.
		 */
		read(0xc); // clear it once
		timevalue_t now = _clock.dest_time();
		uint8_t flags = 0;
		do {
			flags |= read(0xc);
		}
		while(now + MS_TIMEOUT >= _clock.dest_time() && (~flags & 0x10));
		assert(flags & 0x10);
	}

	/**
	 * @return seconds * FREQUENCY since epoch.
	 */
	timevalue_t timestamp() {
		// read out everything
		uint8_t data[14];
		for(size_t i = 0; i < sizeof(data); i++)
			data[i] = read(i);
		// Read the century from the IBM PC location
		// we put it temporarly in the unused weekday field
		data[6] = read(0x32);

		// convert twelve hour format
		if(~data[0xb] & 2) {
			uint8_t hour = data[4] & 0x7f;
			if(~data[0xb] & 4)
				hour = nre::Math::from_bcd(hour);
			hour %= 12;
			if(data[4] & 0x80)
				hour += 12;
			if(~data[0xb] & 4)
				hour = nre::Math::to_bcd(hour);
			data[4] = hour;
		}

		// convert from BCD to binary
		if(~data[0xb] & 4) {
			for(size_t i = 0; i < sizeof(data) && i < 10; i++)
				data[i] = nre::Math::from_bcd(data[i]);
		}

		// Convert to seconds since 1970.
		int year = data[9] + 100 * data[6];
		if(year < 1970)
			year += 100;
		nre::DateInfo date(year,data[8],data[7],data[4],data[2],data[0]);
		return nre::Date::mktime(&date) * nre::Timer::WALLCLOCK_FREQ;
	}

private:
	uint8_t read(uint index) {
		_ports.out<uint8_t>(index,0);
		return _ports.in<uint8_t>(1);
	}

	nre::Ports _ports;
	nre::Clock _clock;
};
