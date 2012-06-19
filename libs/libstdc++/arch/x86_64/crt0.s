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

.global _stack
.global _start
.extern main
.extern exit
.extern _setup
.extern _presetup
.extern _init

_start:
	mov		%rdi, %rbx
	and		$0x7FFFFFFF, %rdi			# remove bit
	mov		$0x80000000, %rax			# the bit says that we are not the root-task
	and		%rax, %rbx
	test	%rbx, %rbx
	jnz		1f
	mov		%rsp, _startup_info			# store pointer to HIP
	lea		-0x1000(%rsp), %rdx		# UTCB is below HIP
	mov		$_stack, %rsp				# switch to our stack
	add		$0x1000, %rsp
	jmp		2f
1:
	mov		%rcx, _startup_info			# store pointer to HIP
2:
	mov		%rdx, _startup_info + 8 	# store pointer to UTCB
	mov		%rsp,%rcx
	sub		$0x1000,%rcx
	mov		%rcx, _startup_info + 16	# store stack-begin
	mov		%rdi, _startup_info + 24	# store cpu
	sub		$16, %rsp					# leave space for Ec and Pd

	# call early setup (current ec and pd)
	call	_presetup
	# call function in .init-section
	call	_init
	# other init stuff
	shr		$31, %rbx
	mov		%rbx, %rdi					# 0 for root, 1 otherwise
	call	_setup
	# finally, call main
	call	main

	mov		%rax, %rdi
	call	exit
	# just to be sure
	1:		jmp	1b

# information for startup
.section .bss.startup_info
.global _startup_info
_startup_info:
	.quad	0	# HIP
	.quad	0	# UTCB
	.quad	0	# thread
	.quad	0	# cpu
	.quad	0	# inited
