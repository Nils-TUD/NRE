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

#include <arch/Startup.h>
#include <services/Admission.h>

namespace nre {

Connection *AdmissionSession::_con = 0;
AdmissionSession::Init AdmissionSession::_init INIT_PRIO_SERIAL;

AdmissionSession::Init::Init() {
	try {
		AdmissionSession::_con = new Connection("admission");
	}
	catch(...) {
		// this happens in root and the admission-service itself
	}
}

}
