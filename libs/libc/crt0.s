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

.section .text

.global _start
.extern start
.extern exit
.extern _setup
.extern _init

_start:
	mov		%esp, _startup_info		# store pointer to HIP
	lea		-0x1000(%esp), %edx		# UTCB is below HIP
	mov		%edx, _startup_info + 4 # store pointer to UTCB
	mov		%eax, _startup_info + 8	# store cpu
	mov		$stack, %esp			# switch to our stack
	sub		$8,%esp					# leave space for Ec and Pd

	# call function in .init-section
	call	_init
	# create initial Pd and Ec
	call	_setup
	# finally, call start
	call	start
	add		$12, %esp

	push	%eax
	call	exit
	# just to be sure
	1:		jmp	1b

# A fast reply to our client, called by a return to a portal
# function.
.global idc_reply_and_wait_fast
idc_reply_and_wait_fast:
	# w0: NOVA_IPC_REPLY
	mov     $1,     %al
	# keep a pointer to ourself on the stack
	# ecx: stack
	lea     -4(%esp), %ecx
	sysenter

# information for startup
.section .bss.startup_info
.global _startup_info
_startup_info:
	.long	0	# HIP
	.long	0	# UTCB
	.long	0	# cpu

.section .bss.stack
.align 0x1000
	.space	0x1000
stack:
