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

#include <subsystem/ChildManager.h>
#include <subsystem/ChildHip.h>
#include <stream/Serial.h>
#include <ipc/Service.h>
#include <kobj/Gsi.h>
#include <kobj/Ports.h>
#include <arch/Elf.h>
#include <util/Math.h>
#include <Logging.h>
#include <new>

namespace nre {

ChildManager::ChildManager()
    : _child_count(), _childs(),
      _portal_caps(CapSelSpace::get().allocate(MAX_CHILDS * per_child_caps(), per_child_caps())),
      _dsm(), _registry(), _sm(), _switchsm(), _slotsm(), _regsm(0), _diesm(0), _ecs(), _regecs() {
    _ecs = new LocalThread *[CPU::count()];
    _regecs = new LocalThread *[CPU::count()];
    for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it) {
        LocalThread **ecs[] = {_ecs, _regecs};
        for(size_t i = 0; i < ARRAY_SIZE(ecs); ++i) {
            ecs[i][it->log_id()] = LocalThread::create(it->log_id());
            ecs[i][it->log_id()]->set_tls(Thread::TLS_PARAM, this);
        }

        UtcbFrameRef defuf(_ecs[it->log_id()]->utcb());
        defuf.accept_translates();
        defuf.accept_delegates(0);

        UtcbFrameRef reguf(_regecs[it->log_id()]->utcb());
        defuf.accept_translates();
        reguf.accept_delegates(Math::next_pow2_shift<size_t>(CPU::count()));
    }
}

ChildManager::~ChildManager() {
    for(size_t i = 0; i < CPU::count(); ++i) {
        Child *c = rcu_dereference(_childs[i]);
        if(c)
            RCU::invalidate(c);
    }
    for(size_t i = 0; i < CPU::count(); ++i) {
        delete _ecs[i];
        delete _regecs[i];
    }
    delete[] _ecs;
    delete[] _regecs;
    CapSelSpace::get().free(MAX_CHILDS * per_child_caps());
    RCU::gc(true);
}

void ChildManager::prepare_stack(Child *c, uintptr_t &sp, uintptr_t csp) {
    /*
     * Initial stack:
     * +------------------+  <- top
     * |     arguments    |
     * |        ...       |
     * +------------------+
     * |         0        |
     * +------------------+
     * |   argv[argc-1]   |
     * +------------------+
     * |       ...        |
     * +------------------+
     * |     argv[0]      | <--\
     * +------------------+    |
     * |       argv       | ---/
     * +------------------+
     * |       argc       |
     * +------------------+
     */

    // first, simply copy the command-line to the stack
    const String &cmdline = c->cmdline();
    size_t len = Math::min<size_t>(MAX_CMDLINE_LEN, cmdline.length());
    char *bottom = reinterpret_cast<char*>(sp - Math::round_up<size_t>(len + 1, sizeof(word_t)));
    memcpy(bottom, cmdline.str(), len + 1);

    // count number of arguments
    size_t i = 0, argc = 0;
    char *str = bottom;
    while(*str) {
        if(*str == ' ' && i > 0)
            argc++;
        else if(*str != ' ')
            i++;
        str++;
    }
    if(i > 0)
        argc++;

    word_t *ptrs = reinterpret_cast<word_t*>(bottom - sizeof(word_t) * (argc + 1));
    // ensure that its aligned to 16-byte for SSE; this time not +8 because we'll call main
    ptrs = reinterpret_cast<word_t*>(reinterpret_cast<uintptr_t>(ptrs) & ~0xFUL);
    // store argv and argc
    *(ptrs - 1) = csp + (reinterpret_cast<word_t>(ptrs) & (ExecEnv::STACK_SIZE - 1));
    *(ptrs - 2) = argc;
    // store stackpointer for user
    sp = csp + (reinterpret_cast<uintptr_t>(ptrs - 2) & (ExecEnv::STACK_SIZE - 1));

    // now, walk through it, replace ' ' by '\0' and store pointers to the individual arguments
    str = bottom;
    i = 0;
    char *begin = bottom;
    while(*str) {
        if(*str == ' ' && i > 0) {
            *ptrs++ = csp + (reinterpret_cast<word_t>(begin) & (ExecEnv::STACK_SIZE - 1));
            *str = '\0';
            i = 0;
        }
        else if(*str != ' ') {
            if(i == 0)
                begin = str;
            i++;
        }
        str++;
    }
    if(i > 0)
        *ptrs++ = csp + (reinterpret_cast<word_t>(begin) & (ExecEnv::STACK_SIZE - 1));
    // terminate
    *ptrs++ = 0;
}

void ChildManager::build_hip(Child *c, const ChildConfig &config) {
    // create ds for cmdlines in Hip mem-items
    uintptr_t cmdlinesaddr = c->reglist().find_free(MAX_MODAUX_LEN);
    const DataSpace &auxds = _dsm.create(
        DataSpaceDesc(MAX_MODAUX_LEN, DataSpaceDesc::ANONYMOUS, DataSpaceDesc::RWX));
    char *cmdlines = reinterpret_cast<char*>(auxds.virt());
    char *cmdlinesend = cmdlines + MAX_MODAUX_LEN;
    c->reglist().add(auxds.desc(), cmdlinesaddr, ChildMemory::R | ChildMemory::OWN, auxds.unmapsel());

    // create ds for hip
    const DataSpace &ds = _dsm.create(
        DataSpaceDesc(ExecEnv::PAGE_SIZE, DataSpaceDesc::ANONYMOUS, DataSpaceDesc::RW));
    c->_hip = c->reglist().find_free(ExecEnv::PAGE_SIZE);
    ChildHip *hip = reinterpret_cast<ChildHip*>(ds.virt());

    // setup Hip
    new (hip)ChildHip(config.cpus());
    HipMem mem;
    for(size_t memidx = 0; config.get_module(memidx, mem); ++memidx) {
        uintptr_t auxaddr = 0;
        if(mem.aux) {
            size_t len = strlen(mem.cmdline()) + 1;
            if(cmdlines + len <= cmdlinesend) {
                memcpy(cmdlines, mem.cmdline(), len);
                auxaddr = cmdlinesaddr;
            }
            cmdlines += len;
            cmdlinesaddr += len;
        }
        hip->add_mem(mem.addr, mem.size, auxaddr, mem.type);
    }
    hip->finalize();

    // add to region list
    c->reglist().add(ds.desc(), c->_hip, ChildMemory::R | ChildMemory::OWN, ds.unmapsel());
}

Child::id_type ChildManager::load(uintptr_t addr, size_t size, const ChildConfig &config) {
    ElfEh *elf = reinterpret_cast<ElfEh*>(addr);

    // check ELF
    if(size < sizeof(ElfEh) || sizeof(ElfPh) > elf->e_phentsize ||
       size < elf->e_phoff + elf->e_phentsize * elf->e_phnum)
        throw ElfException(E_ELF_INVALID, "Size of ELF file invalid");
    if(!(elf->e_ident[0] == 0x7f && elf->e_ident[1] == 'E' &&
         elf->e_ident[2] == 'L' && elf->e_ident[3] == 'F'))
        throw ElfException(E_ELF_SIG, "No ELF signature");

    static int exc[] = {
        CapSelSpace::EV_DIVIDE, CapSelSpace::EV_DEBUG, CapSelSpace::EV_BREAKPOINT,
        CapSelSpace::EV_OVERFLOW, CapSelSpace::EV_BOUNDRANGE, CapSelSpace::EV_UNDEFOP,
        CapSelSpace::EV_NOMATHPROC, CapSelSpace::EV_DBLFAULT, CapSelSpace::EV_TSS,
        CapSelSpace::EV_INVSEG, CapSelSpace::EV_STACK, CapSelSpace::EV_GENPROT,
        CapSelSpace::EV_MATHFAULT, CapSelSpace::EV_ALIGNCHK, CapSelSpace::EV_MACHCHK,
        CapSelSpace::EV_SIMD
    };

    // create child
    size_t idx = free_slot();
    capsel_t pts = _portal_caps + idx * per_child_caps();
    Child *c = new Child(this, pts, config.cmdline());
    try {
        // we have to create the portals first to be able to delegate them to the new Pd
        c->_ptcount = CPU::count() * (ARRAY_SIZE(exc) + Portals::COUNT - 1);
        c->_pts = new Pt *[c->_ptcount];
        memset(c->_pts, 0, c->_ptcount * sizeof(Pt*));
        for(cpu_t cpu = 0; cpu < CPU::count(); ++cpu) {
            size_t idx = cpu * (ARRAY_SIZE(exc) + Portals::COUNT - 1);
            size_t off = cpu * Hip::get().service_caps();
            size_t i = 0;
            for(; i < ARRAY_SIZE(exc); ++i) {
                c->_pts[idx + i] = new Pt(_ecs[cpu], pts + off + exc[i], Portals::exception,
                                          Mtd(Mtd::GPR_BSD | Mtd::QUAL | Mtd::RIP_LEN));
            }
            c->_pts[idx + i++] = new Pt(_ecs[cpu], pts + off + CapSelSpace::EV_PAGEFAULT, Portals::pf,
                                        Mtd(Mtd::GPR_BSD | Mtd::QUAL | Mtd::RIP_LEN));
            c->_pts[idx + i++] = new Pt(_ecs[cpu], pts + off + CapSelSpace::EV_STARTUP, Portals::startup,
                                        Mtd(Mtd::RSP));
            c->_pts[idx + i++] = new Pt(_ecs[cpu], pts + off + CapSelSpace::SRV_INIT, Portals::init_caps,
                                        Mtd(0));
            c->_pts[idx + i++] = new Pt(_regecs[cpu], pts + off + CapSelSpace::SRV_SERVICE,
                                        Portals::service, Mtd(0));
            c->_pts[idx + i++] = new Pt(_ecs[cpu], pts + off + CapSelSpace::SRV_IO, Portals::io, Mtd(0));
            c->_pts[idx + i++] = new Pt(_ecs[cpu], pts + off + CapSelSpace::SRV_SC, Portals::sc, Mtd(0));
            c->_pts[idx + i++] = new Pt(_ecs[cpu], pts + off + CapSelSpace::SRV_GSI, Portals::gsi, Mtd(0));
            c->_pts[idx + i++] = new Pt(_ecs[cpu], pts + off + CapSelSpace::SRV_DS, Portals::dataspace,
                                        Mtd(0));
        }
        // now create Pd and pass portals
        c->_pd = new Pd(Crd(pts, Math::next_pow2_shift(per_child_caps()), Crd::OBJ_ALL));
        c->_entry = elf->e_entry;
        c->_main = config.entry();

        // check load segments and add them to regions
        for(size_t i = 0; i < elf->e_phnum; i++) {
            ElfPh *ph = reinterpret_cast<ElfPh*>(addr + elf->e_phoff + i * elf->e_phentsize);
            if(reinterpret_cast<uintptr_t>(ph) + sizeof(ElfPh) > addr + size)
                throw ElfException(E_ELF_INVALID, "Program header outside binary");
            if(ph->p_type != 1)
                continue;
            if(size < ph->p_offset + ph->p_filesz)
                throw ElfException(E_ELF_INVALID, "LOAD segment outside binary");

            uint perms = ChildMemory::OWN;
            if(ph->p_flags & PF_R)
                perms |= ChildMemory::R;
            if(ph->p_flags & PF_W)
                perms |= ChildMemory::W;
            if(ph->p_flags & PF_X)
                perms |= ChildMemory::X;

            size_t dssize = Math::round_up<size_t>(ph->p_memsz, ExecEnv::PAGE_SIZE);
            // TODO leak, if reglist().add throws
            const DataSpace &ds = _dsm.create(
                DataSpaceDesc(dssize, DataSpaceDesc::ANONYMOUS, DataSpaceDesc::RWX));
            // TODO actually it would be better to do that later
            memcpy(reinterpret_cast<void*>(ds.virt()),
                   reinterpret_cast<void*>(addr + ph->p_offset), ph->p_filesz);
            memset(reinterpret_cast<void*>(ds.virt() + ph->p_filesz), 0, ph->p_memsz - ph->p_filesz);
            c->reglist().add(ds.desc(), ph->p_vaddr, perms, ds.unmapsel());
        }

        // utcb
        c->_utcb = c->reglist().find_free(Utcb::SIZE);
        // just reserve the virtual memory with no permissions; it will not be requested
        c->reglist().add(DataSpaceDesc(Utcb::SIZE, DataSpaceDesc::VIRTUAL, 0), c->_utcb, 0);
        c->_ec = new GlobalThread(reinterpret_cast<GlobalThread::startup_func>(elf->e_entry),
                                  config.cpu(), c->cmdline(), c->_pd, c->_utcb);

        // he needs a stack
        uint align = Math::next_pow2_shift(ExecEnv::STACK_SIZE);
        c->_stack = c->reglist().find_free(ExecEnv::STACK_SIZE, ExecEnv::STACK_SIZE);
        c->reglist().add(DataSpaceDesc(ExecEnv::STACK_SIZE, DataSpaceDesc::ANONYMOUS, 0, 0,
                                       c->_ec->stack(), align - ExecEnv::PAGE_SHIFT),
                         c->stack(), ChildMemory::RW | ChildMemory::OWN);

        // and a Hip
        build_hip(c, config);

        LOG(Logging::CHILD_CREATE, Serial::get() << "Starting child '" << c->cmdline() << "'...\n");
        LOG(Logging::CHILD_CREATE, Serial::get() << *c << "\n");

        // start child; we have to put the child into the list before that
        rcu_assign_pointer(_childs[idx], c);
        c->_ec->start(Qpd(), c->_pd);
    }
    catch(...) {
        delete c;
        rcu_assign_pointer(_childs[idx], 0);
        throw;
    }

    _child_count++;

    // wait until all services are registered
    if(config.waits() > 0) {
        size_t services_present;
        do {
            _regsm.down();
            services_present = 0;
            for(size_t i = 0; i < config.waits(); ++i) {
                if(_registry.find(config.wait(i)))
                    services_present++;
            }
        }
        while(services_present < config.waits());
    }
    return c->id();
}

void ChildManager::Portals::startup(capsel_t pid) {
    ChildManager *cm = Thread::current()->get_tls<ChildManager*>(Thread::TLS_PARAM);
    UtcbExcFrameRef uf;
    try {
        ScopedLock<RCULock> guard(&RCU::lock());
        Child *c = cm->get_child(pid);
        if(c->_started) {
            uintptr_t stack = uf->rsp & ~(ExecEnv::PAGE_SIZE - 1);
            ChildMemory::DS *ds = c->reglist().find_by_addr(stack);
            if(!ds)
                throw ChildMemoryException(E_NOT_FOUND, 64, "Dataspace not found for stack @ %p", stack);
            uf->rip = *reinterpret_cast<word_t*>(
                ds->origin(stack) + (uf->rsp & (ExecEnv::PAGE_SIZE - 1)) + sizeof(word_t));
            uf->mtd = Mtd::RIP_LEN;
        }
        else {
            uf->rip = *reinterpret_cast<word_t*>(uf->rsp + sizeof(word_t));
            prepare_stack(c, uf->rsp, c->stack());
            // the bit indicates that its not the root-task
            // TODO not nice
    #ifdef __i386__
            uf->rax = (1 << 31) | c->_ec->cpu();
    #else
            uf->rdi = (1 << 31) | c->_ec->cpu();
    #endif
            uf->rsi = c->_main;
            uf->rcx = c->hip();
            uf->rdx = c->utcb();
            uf->mtd = Mtd::RIP_LEN | Mtd::RSP | Mtd::GPR_ACDB | Mtd::GPR_BSD;
            c->_started = true;
        }
    }
    catch(...) {
        // let the kernel kill the Thread
        uf->rip = ExecEnv::KERNEL_START;
        uf->mtd = Mtd::RIP_LEN;
    }
}

void ChildManager::Portals::init_caps(capsel_t pid) {
    ChildManager *cm = Thread::current()->get_tls<ChildManager*>(Thread::TLS_PARAM);
    UtcbFrameRef uf;
    try {
        ScopedLock<RCULock> guard(&RCU::lock());
        Child *c = cm->get_child(pid);
        // we can't give the child the cap for e.g. the Pd when creating the Pd. therefore the child
        // grabs them afterwards with this portal
        uf.finish_input();
        // don't allow them to create Sc's
        uf.delegate(c->_pd->sel(), 0, UtcbFrame::NONE, Crd::OBJ | Crd::PD_EC |
                    Crd::PD_PD | Crd::PD_PT | Crd::PD_SM);
        uf.delegate(c->_ec->sel(), 1);
        uf.delegate(c->_ec->sc()->sel(), 2);
        uf << E_SUCCESS;
    }
    catch(const Exception& e) {
        Syscalls::revoke(uf.delegation_window(), true);
        uf.clear();
        uf << e;
    }
}

void ChildManager::Portals::service(capsel_t pid) {
    ChildManager *cm = Thread::current()->get_tls<ChildManager*>(Thread::TLS_PARAM);
    UtcbFrameRef uf;
    try {
        ScopedLock<RCULock> guard(&RCU::lock());
        Child *c = cm->get_child(pid);
        String name;
        Service::Command cmd;
        uf >> cmd;
        if(cmd != Service::CLIENT_DIED)
            uf >> name;
        switch(cmd) {
            case Service::REGISTER: {
                BitField<Hip::MAX_CPUS> available;
                capsel_t cap = uf.get_delegated(uf.delegation_window().order()).offset();
                uf >> available;
                uf.finish_input();

                LOG(Logging::SERVICES,
                    Serial::get() << "Child '" << c->cmdline() << "' regs " << name << "\n");
                capsel_t sm = cm->reg_service(c, cap, name, available);
                uf.accept_delegates();
                uf.delegate(sm);
                uf << E_SUCCESS;
            }
            break;

            case Service::UNREGISTER: {
                uf.finish_input();

                LOG(Logging::SERVICES,
                    Serial::get() << "Child '" << c->cmdline() << "' unregs " << name << "\n");
                cm->unreg_service(c, name);
                uf << E_SUCCESS;
            }
            break;

            case Service::GET: {
                uf.finish_input();

                LOG(Logging::SERVICES,
                    Serial::get() << "Child '" << c->cmdline() << "' gets " << name << "\n");
                const ServiceRegistry::Service* s = cm->get_service(name);

                uf.delegate(CapRange(s->pts(), CPU::count(), Crd::OBJ_ALL));
                uf << E_SUCCESS << s->available();
            }
            break;

            case Service::CLIENT_DIED: {
                uf.finish_input();

                LOG(Logging::SERVICES,
                    Serial::get() << "Child '" << c->cmdline() << "' says clients died\n");
                cm->notify_services();

                uf << E_SUCCESS;
            }
            break;
        }
    }
    catch(const Exception& e) {
        Syscalls::revoke(uf.delegation_window(), true);
        uf.clear();
        uf << e;
    }
}

capsel_t ChildManager::get_parent_service(const char *name, BitField<Hip::MAX_CPUS> &available) {
    UtcbFrame uf;
    ScopedCapSels caps(1 << CPU::order(), 1 << CPU::order());
    uf.delegation_window(Crd(caps.get(), CPU::order(), Crd::OBJ_ALL));
    uf << Service::GET << String(name);
    CPU::current().srv_pt().call(uf);
    uf.check_reply();
    uf >> available;
    return caps.release();
}

void ChildManager::Portals::gsi(capsel_t pid) {
    ChildManager *cm = Thread::current()->get_tls<ChildManager*>(Thread::TLS_PARAM);
    UtcbFrameRef uf;
    try {
        ScopedLock<RCULock> guard(&RCU::lock());
        Child *c = cm->get_child(pid);
        uint gsi;
        void *pcicfg = 0;
        Gsi::Op op;
        uf >> op >> gsi;
        if(op == Gsi::ALLOC)
            uf >> pcicfg;
        uf.finish_input();

        capsel_t cap = 0;
        {
            ScopedLock<UserSm> guard(&c->_sm);
            if(op == Gsi::ALLOC) {
                LOG(Logging::RESOURCES, Serial::get().writef("Child '%s' allocates GSI %u (PCI %p)\n",
                                                             c->cmdline().str(), gsi, pcicfg));
            }
            else {
                LOG(Logging::RESOURCES, Serial::get().writef("Child '%s' releases GSI %u\n",
                                                             c->cmdline().str(), gsi));
            }

            if(gsi >= Hip::MAX_GSIS)
                throw Exception(E_ARGS_INVALID, 32, "Invalid GSI %u", gsi);
            // make sure that just the owner can release it
            if(op == Gsi::RELEASE && !c->gsis().is_set(gsi))
                throw Exception(E_ARGS_INVALID, 32, "Can't release GSI %u. Not owner", gsi);
            if(c->_gsi_next == Hip::MAX_GSIS)
                throw Exception(E_CAPACITY, "No free GSI slots");

            {
                UtcbFrame puf;
                puf << op << gsi;
                if(op == Gsi::ALLOC) {
                    puf << pcicfg;
                    cap = c->_gsi_next++;
                    puf.delegation_window(Crd(c->_gsi_caps + cap, 0, Crd::OBJ_ALL));
                }
                CPU::current().gsi_pt().call(puf);
                puf.check_reply();
                if(op == Gsi::ALLOC)
                    puf >> gsi;
                c->gsis().set(gsi, op == Gsi::ALLOC);
            }
        }

        uf << E_SUCCESS;
        if(op == Gsi::ALLOC) {
            uf << gsi;
            uf.delegate(c->_gsi_caps + cap);
        }
    }
    catch(const Exception& e) {
        Syscalls::revoke(uf.delegation_window(), true);
        uf.clear();
        uf << e;
    }
}

void ChildManager::Portals::io(capsel_t pid) {
    ChildManager *cm = Thread::current()->get_tls<ChildManager*>(Thread::TLS_PARAM);
    UtcbFrameRef uf;
    try {
        ScopedLock<RCULock> guard(&RCU::lock());
        Child *c = cm->get_child(pid);
        Ports::port_t base;
        uint count;
        Ports::Op op;
        uf >> op >> base >> count;
        uf.finish_input();

        {
            ScopedLock<UserSm> guard(&c->_sm);
            if(op == Ports::ALLOC) {
                LOG(Logging::RESOURCES, Serial::get().writef("Child '%s' allocates ports %#x..%#x\n",
                                                             c->cmdline().str(), base, base + count - 1));
            }
            else {
                LOG(Logging::RESOURCES, Serial::get().writef("Child '%s' releases ports %#x..%#x\n",
                                                             c->cmdline().str(), base, base + count - 1));
            }

            // alloc() makes sure that nobody can free something from other childs.
            if(op == Ports::RELEASE)
                c->io().alloc_region(base, count);

            {
                UtcbFrame puf;
                if(op == Ports::ALLOC)
                    puf.delegation_window(Crd(0, 31, Crd::IO_ALL));
                puf << op << base << count;
                CPU::current().io_pt().call(puf);
                puf.check_reply();
            }

            if(op == Ports::ALLOC) {
                c->io().free(base, count);
                uf.delegate(CapRange(base, count, Crd::IO_ALL));
            }
        }
        uf << E_SUCCESS;
    }
    catch(const Exception& e) {
        Syscalls::revoke(uf.delegation_window(), true);
        uf.clear();
        uf << e;
    }
}

void ChildManager::Portals::sc(capsel_t pid) {
    ChildManager *cm = Thread::current()->get_tls<ChildManager*>(Thread::TLS_PARAM);
    UtcbFrameRef uf;
    try {
        ScopedLock<RCULock> guard(&RCU::lock());
        Child *c = cm->get_child(pid);
        Sc::Command cmd;
        uf >> cmd;

        switch(cmd) {
            case Sc::CREATE: {
                uintptr_t stackaddr = 0, utcbaddr = 0;
                bool stack, utcb;
                uf >> stack >> utcb;
                uf.finish_input();

                // TODO we might leak resources here if something fails
                if(stack) {
                    uint align = Math::next_pow2_shift(ExecEnv::STACK_SIZE);
                    DataSpaceDesc desc(ExecEnv::STACK_SIZE, DataSpaceDesc::ANONYMOUS,
                                       DataSpaceDesc::RW, 0, 0, align - ExecEnv::PAGE_SHIFT);
                    const DataSpace &ds = cm->_dsm.create(desc);
                    stackaddr = c->reglist().find_free(ds.size(), ExecEnv::STACK_SIZE);
                    c->reglist().add(ds.desc(), stackaddr, ds.flags() | ChildMemory::OWN, ds.unmapsel());
                }
                if(utcb) {
                    DataSpaceDesc desc(ExecEnv::PAGE_SIZE, DataSpaceDesc::VIRTUAL, DataSpaceDesc::RW);
                    utcbaddr = c->reglist().find_free(ExecEnv::PAGE_SIZE);
                    c->reglist().add(desc, utcbaddr, desc.flags());
                }

                uf << E_SUCCESS;
                if(stack)
                    uf << stackaddr;
                if(utcb)
                    uf << utcbaddr;
            }
            break;

            case Sc::START: {
                String name;
                Qpd qpd;
                cpu_t cpu;
                capsel_t ec = uf.get_delegated(0).offset();
                uf >> name >> cpu >> qpd;
                uf.finish_input();

                // TODO later one could add policy here and adjust the qpd accordingly

                capsel_t sc;
                {
                    UtcbFrame puf;
                    puf.accept_delegates(0);
                    puf << Sc::START << name << cpu << qpd;
                    puf.delegate(ec);
                    CPU::current().sc_pt().call(puf);
                    puf.check_reply();
                    sc = puf.get_delegated(0).offset();
                    puf >> qpd;
                }
                c->add_sc(name, cpu, sc);

                LOG(Logging::ADMISSION,
                    Serial::get().writef("Child '%s' created sc '%s' on cpu %u (%u)\n",
                                         c->cmdline().str(), name.str(), cpu, sc));

                uf.accept_delegates();
                uf.delegate(sc);
                uf << E_SUCCESS << qpd;
            }
            break;

            case Sc::STOP: {
                capsel_t sc = uf.get_translated(0).offset();
                uf.finish_input();

                c->remove_sc(sc);
                {
                    UtcbFrame puf;
                    puf << Sc::STOP;
                    puf.translate(sc);
                    CPU::current().sc_pt().call(puf);
                    puf.check_reply();
                }

                LOG(Logging::ADMISSION, Serial::get().writef("Child '%s' destroyed sc (%u)\n",
                                                             c->cmdline().str(), sc));
            }
            break;
        }
    }
    catch(const Exception& e) {
        Syscalls::revoke(uf.delegation_window(), true);
        uf.clear();
        uf << e;
    }
}

void ChildManager::map(UtcbFrameRef &uf, Child *c, DataSpace::RequestType type) {
    capsel_t sel = 0;
    DataSpaceDesc desc;
    if(type == DataSpace::JOIN)
        sel = uf.get_translated(0).offset();
    else
        uf >> desc;
    uf.finish_input();

    ScopedLock<UserSm> guard(&c->_sm);
    uintptr_t addr = 0;
    if(type != DataSpace::JOIN && desc.type() == DataSpaceDesc::VIRTUAL) {
        addr = c->reglist().find_free(desc.size());
        desc = DataSpaceDesc(desc.size(), desc.type(), desc.flags(), 0, 0, desc.align());
        c->reglist().add(desc, addr, desc.flags());
        desc.virt(addr);
        LOG(Logging::DATASPACES,
            Serial::get() << "Child '" << c->cmdline() << "' allocated virtual ds:\n\t" << desc << "\n");
        uf << E_SUCCESS << desc;
    }
    else {
        // create it or attach to the existing dataspace
        const DataSpace &ds = type == DataSpace::JOIN ? _dsm.join(sel) : _dsm.create(desc);

        // add it to the regions of the child
        try {
            uint flags = ds.flags();
            // only create creations and non-device-memory
            if(type != DataSpace::JOIN && desc.phys() == 0)
                flags |= ChildMemory::OWN;
            size_t align = 1 << (desc.align() + ExecEnv::PAGE_SHIFT);
            addr = c->reglist().find_free(ds.size(), align);
            c->reglist().add(ds.desc(), addr, flags, ds.unmapsel());
        }
        catch(...) {
            _dsm.release(desc, ds.unmapsel());
            throw;
        }

        // build answer
        if(type == DataSpace::CREATE) {
            LOG(Logging::DATASPACES,
                Serial::get() << "Child '" << c->cmdline() << "' created:\n\t" << ds << "\n");
            uf.delegate(ds.sel(), 0);
            uf.delegate(ds.unmapsel(), 1);
        }
        else {
            LOG(Logging::DATASPACES,
                Serial::get() << "Child '" << c->cmdline() << "' joined:\n\t" << ds << "\n");
            uf.accept_delegates();
            uf.delegate(ds.unmapsel());
        }
        uf << E_SUCCESS << DataSpaceDesc(ds.size(), ds.type(), ds.flags(), ds.phys(), addr, ds.virt(),
                                         ds.desc().align());
    }
}

void ChildManager::switch_to(UtcbFrameRef &uf, Child *c) {
    capsel_t srcsel = uf.get_translated(0).offset();
    capsel_t dstsel = uf.get_translated(0).offset();
    uf.finish_input();

    {
        // note that we need another lock here since it may also involve childs of c (c may have
        // delegated it and if they cause a pagefault during this operation, we might get mixed
        // results)
        ScopedLock<UserSm> guard_switch(&_switchsm);

        uintptr_t srcorg, dstorg;
        {
            // first do the stuff for the child that requested the switch
            ScopedLock<UserSm> guard_regs(&c->_sm);
            ChildMemory::DS *src, *dst;
            src = c->reglist().find(srcsel);
            dst = c->reglist().find(dstsel);
            LOG(Logging::DATASPACES, Serial::get() << "Child '" << c->cmdline()
                                                   << "' switches:\n\t" << src->desc() << "\n\t" <<
                dst->desc() << "\n");
            if(!src || !dst)
                throw Exception(E_ARGS_INVALID, 64, "Unable to switch. DS %u or %u not found", srcsel,
                                dstsel);
            if(src->desc().size() != dst->desc().size()) {
                throw Exception(E_ARGS_INVALID, 64,
                                "Unable to switch non-equal-sized dataspaces (%zu,%zu)",
                                src->desc().size(), dst->desc().size());
            }

            // first revoke the memory to prevent further accesses
            CapRange(src->desc().origin() >> ExecEnv::PAGE_SHIFT,
                     src->desc().size() >> ExecEnv::PAGE_SHIFT, Crd::MEM_ALL).revoke(false);
            CapRange(dst->desc().origin() >> ExecEnv::PAGE_SHIFT,
                     dst->desc().size() >> ExecEnv::PAGE_SHIFT, Crd::MEM_ALL).revoke(false);
            // we have to reset the last pf information here, because of the revoke. otherwise it
            // can happen that last time CPU X caused the last fault and this time, CPU X causes
            // the second fault (the first one will handle it and the second one will find it already
            // mapped). therefore, it might be the same address and same CPU again which caused an
            // "already-mapped" fault again and thus, it would be killed.
            c->_last_fault_addr = 0;
            c->_last_fault_cpu = 0;
            // now copy the content
            memcpy(reinterpret_cast<char*>(dst->desc().origin()),
                   reinterpret_cast<char*>(src->desc().origin()),
                   src->desc().size());
            // change mapping
            srcorg = src->desc().origin();
            dstorg = dst->desc().origin();
            src->desc().origin(dstorg);
            dst->desc().origin(srcorg);
            src->all_perms(0);
            dst->all_perms(0);
        }

        // now change the mapping for all other childs that have one of these dataspaces
        for(size_t x = 0, i = 0; i < MAX_CHILDS && x < _child_count; ++i) {
            Child *ch = rcu_dereference(_childs[i]);
            if(ch == 0 || ch == c)
                continue;

            ScopedLock<UserSm> guard_regs(&ch->_sm);
            DataSpaceDesc dummy;
            ChildMemory::DS *src, *dst;
            src = ch->reglist().find(srcsel);
            dst = ch->reglist().find(dstsel);
            if(!src && !dst)
                continue;

            // also reset the information here
            ch->_last_fault_addr = 0;
            ch->_last_fault_cpu = 0;
            if(src) {
                src->desc().origin(dstorg);
                src->all_perms(0);
            }
            if(dst) {
                dst->desc().origin(srcorg);
                src->all_perms(0);
            }
            x++;
        }

        // now swap the origins also in the dataspace-manager (otherwise clients that join
        // afterwards will receive the wrong location)
        _dsm.swap(srcsel, dstsel);
    }

    uf << E_SUCCESS;
}

void ChildManager::unmap(UtcbFrameRef &uf, Child *c) {
    capsel_t sel = 0;
    DataSpaceDesc desc;
    uf >> desc;
    if(desc.type() != DataSpaceDesc::VIRTUAL)
        sel = uf.get_translated(0).offset();
    uf.finish_input();

    ScopedLock<UserSm> guard(&c->_sm);
    if(desc.type() == DataSpaceDesc::VIRTUAL) {
        LOG(Logging::DATASPACES, Serial::get() << "Child '" << c->cmdline()
                                               << "' destroys virtual ds " << desc << "\n");
        c->reglist().remove_by_addr(desc.virt());
    }
    else {
        LOG(Logging::DATASPACES, Serial::get() << "Child '" << c->cmdline()
                                               << "' destroys " << sel << ": " << desc << "\n");
        // destroy (decrease refs) the ds
        _dsm.release(desc, sel);
        c->reglist().remove(sel);
    }
    uf << E_SUCCESS;
}

void ChildManager::Portals::dataspace(capsel_t pid) {
    ChildManager *cm = Thread::current()->get_tls<ChildManager*>(Thread::TLS_PARAM);
    UtcbFrameRef uf;
    try {
        ScopedLock<RCULock> guard(&RCU::lock());
        DataSpace::RequestType type;
        Child *c = cm->get_child(pid);
        uf >> type;

        switch(type) {
            case DataSpace::CREATE:
            case DataSpace::JOIN:
                cm->map(uf, c, type);
                break;

            case DataSpace::SWITCH_TO:
                cm->switch_to(uf, c);
                break;

            case DataSpace::DESTROY:
                cm->unmap(uf, c);
                break;
        }
    }
    catch(const Exception& e) {
        Syscalls::revoke(uf.delegation_window(), true);
        uf.clear();
        uf << e;
    }
}

void ChildManager::Portals::pf(capsel_t pid) {
    ChildManager *cm = Thread::current()->get_tls<ChildManager*>(Thread::TLS_PARAM);
    UtcbExcFrameRef uf;
    cpu_t cpu = cm->get_cpu(pid);

    bool kill = false;
    uintptr_t pfaddr = uf->qual[1];
    unsigned error = uf->qual[0];
    uintptr_t eip = uf->rip;

    // voluntary exit?
    if(pfaddr == eip && pfaddr >= ExecEnv::EXIT_START && pfaddr <= ExecEnv::THREAD_EXIT) {
        cm->term_child(pid, uf);
        return;
    }

    try {
        ScopedLock<RCULock> guard(&RCU::lock());
        Child *c = cm->get_child(pid);
        ScopedLock<UserSm> guard_switch(&cm->_switchsm);
        ScopedLock<UserSm> guard_regs(&c->_sm);

        LOG(Logging::PFS,
            Serial::get().writef("Child '%s': Pagefault for %p @ %p on cpu %u, error=%#x\n",
                                 c->cmdline().str(), reinterpret_cast<void*>(pfaddr),
                                 reinterpret_cast<void*>(eip), cpu, error));

        // TODO different handlers (cow, ...)
        uintptr_t pfpage = pfaddr & ~(ExecEnv::PAGE_SIZE - 1);
        bool remap = false;
        ChildMemory::DS *ds = c->reglist().find_by_addr(pfaddr);
        uint perms = 0;
        uint flags = 0;
        kill = !ds || !ds->desc().flags();
        if(!kill) {
            flags = ds->page_perms(pfaddr);
            perms = ds->desc().flags() & ChildMemory::RWX;
        }
        // check if the access rights are violated
        if(flags) {
            if((error & 0x2) && !(perms & ChildMemory::W))
                kill = true;
            if((error & 0x4) && !(perms & ChildMemory::R))
                kill = true;
        }

        // is the page already mapped (may be ok if two cpus accessed the page at the same time)
        if(!kill && flags) {
            // first check if our parent has unmapped the memory
            Crd res = Syscalls::lookup(Crd(ds->origin(pfaddr) >> ExecEnv::PAGE_SHIFT, 0, Crd::MEM));
            // if so, remap it
            if(res.is_null()) {
                // reset it here as well. this is necessary for subsystems (where our parent has
                // revoked the memory)
                c->_last_fault_addr = 0;
                c->_last_fault_cpu = 0;
                // reset all permissions since we want to remap it completely.
                // note that this assumes that we're revoking always complete dataspaces.
                ds->all_perms(0);
                remap = true;
            }
            // same fault for same cpu again?
            else if(pfpage == c->_last_fault_addr && cpu == c->_last_fault_cpu) {
                LOG(Logging::CHILD_KILL, Serial::get().writef(
                        "Child '%s': Caused fault for %p on cpu %u twice. Giving up :(\n",
                        c->cmdline().str(), reinterpret_cast<void*>(pfaddr), CPU::get(cpu).phys_id()));
                kill = true;
            }
            else {
                LOG(Logging::PFS, Serial::get().writef(
                        "Child '%s': Pagefault for %p @ %p on cpu %u, error=%#x (page already mapped)\n",
                        c->cmdline().str(), reinterpret_cast<void*>(pfaddr),
                        reinterpret_cast<void*>(eip), CPU::get(cpu).phys_id(), error));
                LOG(Logging::PFS_DETAIL, Serial::get() << "See regionlist:\n" << c->reglist());
                c->_last_fault_addr = pfpage;
                c->_last_fault_cpu = cpu;
            }
        }

        if(!kill && (remap || !flags)) {
            // try to map the next few pages
            size_t pages = 32;
            if(ds->desc().flags() & DataSpaceDesc::BIGPAGES) {
                // try to map the whole pagetable at once
                pages = ExecEnv::PT_ENTRY_COUNT;
                // take care that we start at the beginning (note that this assumes that it is
                // properly aligned, which is made sure by root. otherwise we might leave the ds
                pfpage &= ~(ExecEnv::BIG_PAGE_SIZE - 1);
            }
            uintptr_t src = ds->origin(pfpage);
            CapRange cr(src >> ExecEnv::PAGE_SHIFT, pages, Crd::MEM | (perms << 2),
                        pfpage >> ExecEnv::PAGE_SHIFT);
            // ensure that it fits into the utcb
            cr.limit_to(uf.free_typed());
            cr.count(ds->page_perms(pfpage, cr.count(), perms));
            uf.delegate(cr);
            // ensure that we have the memory (if we're a subsystem this might not be true)
            // TODO this is not sufficient, in general
            // TODO perhaps we could find the dataspace, that belongs to this address and use this
            // one to notify the parent that he should map it?
            UNUSED volatile int x = *reinterpret_cast<int*>(src);
        }
    }
    catch(...) {
        kill = true;
    }

    // we can't release the lock after having killed the child. thus, we do out here (it's save
    // because there can't be any running Ecs anyway since we only destroy it when there are no
    // other Ecs left)
    if(kill) {
        try {
            {
                ScopedLock<RCULock> guard(&RCU::lock());
                Child *c = cm->get_child(pid);
                LOG(Logging::CHILD_KILL, Serial::get().writef(
                        "Child '%s': Unresolvable pagefault for %p @ %p on cpu %u, error=%#x\n",
                        c->cmdline().str(), reinterpret_cast<void*>(pfaddr),
                        reinterpret_cast<void*>(uf->rip), CPU::get(cpu).phys_id(), error));
            }
            cm->kill_child(pid, uf, FAULT);
        }
        catch(...) {
            // just let the kernel kill the Ec here
            uf->mtd = Mtd::RIP_LEN;
            uf->rip = ExecEnv::KERNEL_START;
        }
    }
}

void ChildManager::Portals::exception(capsel_t pid) {
    ChildManager *cm = Thread::current()->get_tls<ChildManager*>(Thread::TLS_PARAM);
    UtcbExcFrameRef uf;
    cm->kill_child(pid, uf, FAULT);
}

void ChildManager::term_child(capsel_t pid, UtcbExcFrameRef &uf) {
    try {
        bool pd = uf->eip != ExecEnv::THREAD_EXIT;
        {
            ScopedLock<RCULock> guard(&RCU::lock());
            Child *c = get_child(pid);
            // lol: using the condition operator instead of if-else leads to
            // "undefined reference to `nre::ExecEnv::THREAD_EXIT'"
            int exitcode = uf->eip;
            if(pd)
                exitcode -= ExecEnv::EXIT_START;
            else
                exitcode -= ExecEnv::THREAD_EXIT;
            LOG(Logging::CHILD_KILL, Serial::get().writef(
                    "Child '%s': %s terminated with exit code %d on cpu %u\n",
                    c->cmdline().str(), pd ? "Pd" : "Thread", exitcode, get_cpu(pid)));
        }

        kill_child(pid, uf, pd ? PROC_EXIT : THREAD_EXIT);
    }
    catch(...) {
        // just let the kernel kill the Ec here
        uf->mtd = Mtd::RIP_LEN;
        uf->rip = ExecEnv::KERNEL_START;
    }
}

void ChildManager::kill_child(capsel_t pid, UtcbExcFrameRef &uf, ExitType type) {
    bool dead = false;
    try {
        ScopedLock<RCULock> guard(&RCU::lock());
        Child *c = get_child(pid);
        uintptr_t *addr, addrs[32];
        if(type == FAULT) {
            LOG(Logging::CHILD_KILL, Serial::get().writef(
                    "Child '%s': caused exception %u @ %p on cpu %u\n",
                    c->cmdline().str(), get_vector(pid), reinterpret_cast<void*>(uf->rip),
                    CPU::get(get_cpu(pid)).phys_id()));
            LOG(Logging::CHILD_KILL, Serial::get() << c->reglist());
            LOG(Logging::CHILD_KILL, Serial::get().writef("Unable to resolve fault; killing child\n"));
        }
        // if its a thread exit, free stack and utcb
        else if(type == THREAD_EXIT) {
            ScopedLock<UserSm> guard(&c->_sm);
            uintptr_t stack = uf->rsi;
            uintptr_t utcb = uf->rdi;
            // 0 indicates that this thread has used its own stack
            if(stack) {
                capsel_t sel;
                DataSpaceDesc desc = c->reglist().remove_by_addr(stack, &sel);
                _dsm.release(desc, sel);
            }
            if(utcb)
                c->reglist().remove_by_addr(utcb);
        }
        ExecEnv::collect_backtrace(c->_ec->stack(), uf->rbp, addrs, 32);
        LOG(Logging::CHILD_KILL, Serial::get().writef("Backtrace:\n"));
        addr = addrs;
        while(*addr != 0) {
            LOG(Logging::CHILD_KILL, Serial::get().writef("\t%p\n", reinterpret_cast<void*>(*addr)));
            addr++;
        }
    }
    catch(const ChildMemoryException &e) {
        // this happens if removing the stack or utcb fails. we consider this as a protocol violation
        // of the child and thus kill it.
        LOG(Logging::CHILD_KILL, Serial::get() << "Child thread violated exit protocol ("
                                               << e.msg() << "); killing it\n");
        type = FAULT;
    }
    catch(...) {
        // ignore the exception here. this may happen if the child is already gone
        dead = true;
    }

    // let the kernel kill the Thread by causing it a pagefault in kernel-area
    uf->mtd = Mtd::RIP_LEN;
    uf->rip = ExecEnv::KERNEL_START;
    if(!dead && type != THREAD_EXIT)
        destroy_child(pid);
}

void ChildManager::destroy_child(capsel_t pid) {
    // we need the lock here to prevent that the slot is reused before we're finished with
    // destroying the child (so, the lock is also used in free_slot())
    ScopedLock<UserSm> guard(&_slotsm);
    size_t i = (pid - _portal_caps) / per_child_caps();
    Child *c = rcu_dereference(_childs[i]);
    if(!c)
        return;
    rcu_assign_pointer(_childs[i], 0);
    _registry.remove(c);
    RCU::invalidate(c);
    // we have to wait until its deleted here because before that we can't reuse the slot.
    // (we need new portals at the same place, so that they have to be revoked first)
    RCU::gc(true);
    _child_count--;
    notify_services();
    Sync::memory_barrier();
    _diesm.up();
}

}
