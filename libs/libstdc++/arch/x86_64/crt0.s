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
.extern _post_init
.extern _init

# initial state for root:
#   rdi: cpu
#   rsp: pointer to Hip

# initial state for non-root:
#   rdi: (1 << 31) | cpu
#   rsp: stack-pointer
#   rcx: pointer to Hip
#   rdx: pointer to Utcb
#   rsi: 0 to call main, otherwise the address of the function to call

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
	sub		$16, %rsp					# leave space for Ec and Pd
	jmp		2f
1:
	mov		%rcx, _startup_info			# store pointer to HIP
	movq	$1, _startup_info + 40		# store that we're a child
2:
	mov		%rdx, _startup_info + 8 	# store pointer to UTCB
	mov		%rsp,%rcx
	sub		$0x1000,%rcx
	mov		%rcx, _startup_info + 16	# store stack-begin
	mov		%rdi, _startup_info + 24	# store cpu
	push	%rsi						# save rsi for later usage

	# call function in .init-section
	call	_init
	call	_post_init
	
	# test whether we're root
	test	%rbx, %rbx
	jnz		1f
	# yes, so call main
	add		$8, %rsp
	call	main
1:
	# check whether we should call a different function
	pop		%rax
	test	%rax, %rax
	mov		(%rsp),%rdi				# argc
	mov		8(%rsp),%rsi				# argv
	jnz		2f
	call	main
	jmp		3f
2:
	call	*%rax

3:
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
	.quad	0	# child
