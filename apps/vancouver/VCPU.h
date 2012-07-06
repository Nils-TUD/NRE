/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

using namespace nul;

class VCPU {
	struct Portal {
		capsel_t offset;
		Pt::portal_func func;
		uint mtd;
	};

public:
#if 0
	static unsigned create_vcpu(VCpu *vcpu,bool use_svm,unsigned cpunr) {
		// create worker
		unsigned cap_worker = create_ec4pt(vcpu,cpunr,
		//Config::EXC_PORTALS*cpunr /* Use s0 exception portals */
		        _pt_irq);

		// create portals for VCPU faults
#undef VM_FUNC
#define VM_FUNC(NR, NAME, INPUT, CODE) {NR, NAME, INPUT},
		struct vm_caps {
			unsigned nr;
			void __attribute__((regparm(1))) (*func)(unsigned,VCpu *,Utcb *);
			unsigned mtd;
		} vm_caps[] = {
#include "vancouver.cc"
		        };
		unsigned cap_start = alloc_cap(0x100);
		for(unsigned i = 0; i < sizeof(vm_caps) / sizeof(vm_caps[0]); i++) {
			if(use_svm == (vm_caps[i].nr < PT_SVM))
				continue;
			check1(0,nova_create_pt(cap_start + (vm_caps[i].nr & 0xff), cap_worker, reinterpret_cast<unsigned long>(vm_caps[i].func), vm_caps[i].mtd));
		}

		Logging::printf("\tcreate VCPU\n");
		unsigned cap_block = alloc_cap(2);
		if(nova_create_sm(cap_block))
			Logging::panic("could not create blocking semaphore\n");
		if(nova_create_ec(cap_block + 1,0,0,cpunr,cap_start,false))
			Logging::panic("creating a VCPU failed - does your CPU support VMX/SVM?");
		AdmissionProtocol::sched sched; //Qpd(1, 10000)
		if(service_admission->alloc_sc(*myutcb(),cap_block + 1,sched,cpunr,"vcpu"))
			Logging::panic("creating a VCPU failed - admission test issue");
		return cap_block;
	}
#endif

	static void force_invalid_gueststate_amd(UtcbExcFrameRef &uf) {
		uf->ctrl[1] = 0;
		uf->mtd = Mtd::CTRL;
	}

	static void force_invalid_gueststate_intel(UtcbExcFrameRef &uf) {
		assert(uf->mtd & Mtd::RFLAGS);
		uf->efl &= ~2;
		uf->mtd = Mtd::RFLAGS;
	}

	static void skip_instruction(CpuMessage &msg) {
		// advance EIP
		assert(msg.mtr_in & Mtd::RIP_LEN);
		msg.cpu->eip += msg.cpu->inst_len;
		msg.mtr_out |= Mtd::RIP_LEN;

		// cancel sti and mov-ss blocking as we emulated an instruction
		assert(msg.mtr_in & Mtd::STATE);
		if(msg.cpu->intr_state & 3) {
			msg.cpu->intr_state &= ~3;
			msg.mtr_out |= Mtd::STATE;
		}
	}

	static void handle_io(bool is_in,unsigned io_order,unsigned port) {
		UtcbExcFrameRef uf;
		VCpu *vcpu = Ec::current()->get_tls<VCpu*>(Ec::TLS_PARAM);

		CpuMessage msg(is_in,static_cast<CpuState *>(Ec::current()->utcb()),io_order,port,&uf->eax,uf->mtd);
		skip_instruction(msg);
		{
			SemaphoreGuard l(_lock);
			if(!vcpu->executor.send(msg,true))
				Util::panic("nobody to execute %s at %x:%x\n",__func__,msg.cpu->cs.sel,msg.cpu->eip);
		}
		/* TODO if(service_events && !msg.consumed)
			service_events->send_event(*utcb,EventsProtocol::EVENT_UNSERVED_IOACCESS,sizeof(port),
			        &port);*/
	}

	static void handle_vcpu(capsel_t pid,bool skip,CpuMessage::Type type) {
		UtcbExcFrameRef uf;
		VCpu *vcpu = Ec::current()->get_tls<VCpu*>(Ec::TLS_PARAM);

		assert(vcpu);
		CpuMessage msg(type,static_cast<CpuState*>(Ec::current()->utcb()),uf->mtd);
		if(skip)
			skip_instruction(msg);

		SemaphoreGuard l(_lock);

		/**
		 * Send the message to the VCpu.
		 */
		if(!vcpu->executor.send(msg,true))
			Util::panic("nobody to execute %s at %x:%x pid %d\n",__func__,msg.cpu->cs.sel,
			        msg.cpu->eip,pid);

		/**
		 * Check whether we should inject something...
		 */
		if((msg.mtr_in & Mtd::INJ) && msg.type != CpuMessage::TYPE_CHECK_IRQ) {
			msg.type = CpuMessage::TYPE_CHECK_IRQ;
			if(!vcpu->executor.send(msg,true))
				Util::panic("nobody to execute %s at %x:%x pid %d\n",__func__,msg.cpu->cs.sel,
				        msg.cpu->eip,pid);
		}

		/**
		 * If the IRQ injection is performed, recalc the IRQ window.
		 */
		if(msg.mtr_out & Mtd::INJ) {
			vcpu->inj_count++;

			msg.type = CpuMessage::TYPE_CALC_IRQWINDOW;
			if(!vcpu->executor.send(msg,true))
				Util::panic("nobody to execute %s at %x:%x pid %d\n",__func__,msg.cpu->cs.sel,
				        msg.cpu->eip,pid);
		}
		msg.cpu->mtd = msg.mtr_out;
	}

	PORTAL static void vmx_triple(capsel_t pid) {
		handle_vcpu(pid,false,CpuMessage::TYPE_TRIPLE);
	}
	PORTAL static void vmx_init(capsel_t pid) {
		handle_vcpu(pid,false,CpuMessage::TYPE_INIT);
	}
	PORTAL static void vmx_irqwin(capsel_t pid) {
	  	COUNTER_INC("irqwin");
		handle_vcpu(pid,false,CpuMessage::TYPE_CHECK_IRQ);
	}
	PORTAL static void vmx_cpuid(capsel_t pid) {
		COUNTER_INC("cpuid");
		bool res = false;
		/* TODO if(_donor_net)
			res = handle_donor_request(utcb,pid,tls);*/
		if(!res)
			handle_vcpu(pid,true,CpuMessage::TYPE_CPUID);
	}
	PORTAL static void vmx_hlt(capsel_t pid) {
		handle_vcpu(pid,true,CpuMessage::TYPE_HLT);
	}
	PORTAL static void vmx_rdtsc(capsel_t pid) {
		COUNTER_INC("rdtsc");
		handle_vcpu(pid,true,CpuMessage::TYPE_RDTSC);
	}
	PORTAL static void vmx_vmcall(capsel_t pid) {
		UtcbExcFrameRef uf;
		uf->eip += uf->inst_len;
	}
	PORTAL static void vmx_ioio(capsel_t pid) {
		UtcbExcFrameRef uf;
		if(uf->qual[0] & 0x10) {
			COUNTER_INC("IOS");
			force_invalid_gueststate_intel(utcb);
		}
		else {
			unsigned order = uf->qual[0] & 7;
			if(order > 2)
				order = 2;
			handle_io(uf->qual[0] & 8,order,uf->qual[0] >> 16);
		}
	}
	PORTAL static void vmx_rdmsr(capsel_t pid) {
		COUNTER_INC("rdmsr");
		handle_vcpu(pid,true,CpuMessage::TYPE_RDMSR);
	}
	PORTAL static void vmx_wrmsr(capsel_t pid) {
		COUNTER_INC("wrmsr");
		handle_vcpu(pid,true,CpuMessage::TYPE_WRMSR);
	}
	PORTAL static void vmx_invalid(capsel_t pid) {
		UtcbExcFrameRef uf;
	  	uf->efl |= 2;
		handle_vcpu(pid,false,CpuMessage::TYPE_SINGLE_STEP);
	  	uf->mtd |= Mtd::RFLAGS;
	}
	PORTAL static void vmx_pause(capsel_t pid) {
		UtcbExcFrameRef uf;
		CpuMessage msg(CpuMessage::TYPE_SINGLE_STEP,static_cast<CpuState*>(Ec::current()->utcb()),uf->mtd);
		skip_instruction(msg);
		COUNTER_INC("pause");
	}
	PORTAL static void vmx_mmio(capsel_t pid) {
	  	COUNTER_INC("MMIO");
	  	/**
	  	 * Idea: optimize the default case - mmio to general purpose register
	  	 * Need state: GPR_ACDB, GPR_BSD, RIP_LEN, RFLAGS, CS, DS, SS, ES, RSP, CR, EFER
	  	 */
		if(!map_memory_helper(tls,utcb,utcb->qual[0] & 0x38))
			// this is an access to MMIO
			handle_vcpu(pid,false,CpuMessage::TYPE_SINGLE_STEP);
	}
	PORTAL static void vmx_startup(capsel_t pid) {
		UtcbExcFrameRef uf;
		Serial::get().writef("startup\n");
		handle_vcpu(pid,false,CpuMessage::TYPE_HLT);
		uf->mtd |= Mtd::CTRL;
		uf->ctrl[0] = 0;
		if(_tsc_offset)
			uf->ctrl[0] |= (1 << 3 /* tscoff */);
		if(_rdtsc_exit)
			uf->ctrl[0] |= (1 << 12 /* rdtsc */);
		uf->ctrl[1] = 0;
	}
	PORTAL static void do_recall(capsel_t pid) {
		UtcbExcFrameRef uf;
	  	COUNTER_INC("recall");
	  	COUNTER_SET("REIP", uf->eip);
	  	handle_vcpu(pid,false,CpuMessage::TYPE_CHECK_IRQ);
	}

	PORTAL static void svm_vintr(capsel_t pid) {
		vmx_irqwin(pid);
	}
	PORTAL static void svm_cpuid(capsel_t pid) {
		UtcbExcFrameRef uf;
		uf->inst_len = 2;
		vmx_cpuid(pid);
	}
	PORTAL static void svm_hlt(capsel_t pid) {
		UtcbExcFrameRef uf;
		uf->inst_len = 1;
		vmx_hlt(pid);
	}
	PORTAL static void svm_ioio(capsel_t pid) {
		UtcbExcFrameRef uf;
		if(uf->qual[0] & 0x4) {
			COUNTER_INC("IOS");
			force_invalid_gueststate_amd(utcb);
		}
		else {
			unsigned order = ((uf->qual[0] >> 4) & 7) - 1;
			if(order > 2)
				order = 2;
			uf->inst_len = uf->qual[1] - uf->eip;
			handle_io(uf->qual[0] & 1,order,uf->qual[0] >> 16);
		}
	}
	PORTAL static void svm_msr(capsel_t pid) {
		svm_invalid(pid);
	}
	PORTAL static void svm_shutdwn(capsel_t pid) {
		vmx_triple(pid);
	}
	PORTAL static void svm_npt(capsel_t pid) {
		UtcbExcFrameRef uf;
		if(!map_memory_helper(uf->qual[0] & 1))
			svm_invalid(pid);
	}
	PORTAL static void svm_invalid(capsel_t pid) {
		UtcbExcFrameRef uf;
		COUNTER_INC("invalid");
		handle_vcpu(pid,false,CpuMessage::TYPE_SINGLE_STEP);
		uf->mtd |= Mtd::CTRL;
		uf->ctrl[0] = 1 << 18; // cpuid
		uf->ctrl[1] = 1 << 0; // vmrun
	}
	PORTAL static void svm_startup(capsel_t pid) {
		vmx_irqwin(pid);
	}
	PORTAL static void svm_recall(capsel_t pid) {
		do_recall(pid);
	}

private:
	bool _tsc_offset = false;
	bool _rdtsc_exit;
	static Portal portals[] = {
		// the VMX portals
		{PT_VMX + 2,	vmx_triple,		Mtd::ALL},
		{PT_VMX + 3,	vmx_init,		Mtd::ALL},
		{PT_VMX + 7,	vmx_irqwin,		Mtd::IRQ},
		{PT_VMX + 10,	vmx_cpuid,		Mtd::RIP_LEN | Mtd::GPR_ACDB | Mtd::STATE | Mtd::CR | Mtd::IRQ},
		{PT_VMX + 12,	vmx_hlt,		Mtd::RIP_LEN | Mtd::IRQ},
		{PT_VMX + 16,	vmx_rdtsc,		Mtd::RIP_LEN | Mtd::GPR_ACDB | Mtd::TSC | Mtd::STATE},
		{PT_VMX + 18,	vmx_vmcall,		Mtd::RIP_LEN},
		{PT_VMX + 30,	vmx_ioio,		Mtd::RIP_LEN | Mtd::QUAL | Mtd::GPR_ACDB | Mtd::STATE | Mtd::RFLAGS},
		{PT_VMX + 31,	vmx_rdmsr,		Mtd::RIP_LEN | Mtd::GPR_ACDB | Mtd::TSC | Mtd::SYSENTER | Mtd::STATE},
		{PT_VMX + 32,	vmx_wrmsr,		Mtd::RIP_LEN | Mtd::GPR_ACDB | Mtd::SYSENTER | Mtd::STATE | Mtd::TSC},
		{PT_VMX + 33,	vmx_invalid,	Mtd::ALL},
		{PT_VMX + 40,	vmx_pause,		Mtd::RIP_LEN | Mtd::STATE},
		{PT_VMX + 48,	vmx_mmio,		Mtd::ALL},
		{PT_VMX + 0xfe,	vmx_startup,	Mtd::IRQ},
	#ifdef EXPERIMENTAL
		{PT_VMX + 0xff,	do_recall,		Mtd::IRQ},
	#else
		{PT_VMX + 0xff,	do_recall,		Mtd::IRQ | Mtd::RIP_LEN | Mtd::GPR_BSD | Mtd::GPR_ACDB},
	#endif
		// the SVM portals
		{PT_SVM + 0x64,	svm_vintr,		Mtd::IRQ},
		{PT_SVM + 0x72,	svm_cpuid,		Mtd::RIP_LEN | Mtd::GPR_ACDB | Mtd::IRQ | Mtd::CR},
		{PT_SVM + 0x78,	svm_hlt,		Mtd::RIP_LEN | Mtd::IRQ},
		{PT_SVM + 0x7b,	svm_ioio,		Mtd::RIP_LEN | Mtd::QUAL | Mtd::GPR_ACDB | Mtd::STATE},
		{PT_SVM + 0x7c,	svm_msr,		Mtd::ALL},
		{PT_SVM + 0x7f,	svm_shutdwn,	Mtd::ALL},
		{PT_SVM + 0xfc,	svm_npt,		Mtd::ALL},
		{PT_SVM + 0xfd,	svm_invalid,	Mtd::ALL},
		{PT_SVM + 0xfe,	svm_startup,	Mtd::ALL},
		{PT_SVM + 0xff,	svm_recall,		Mtd::IRQ},
	};
};
