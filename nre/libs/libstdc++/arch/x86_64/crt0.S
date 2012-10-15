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

#include <arch/Defines.h>

.section .text

.global _stack
.global _start
.extern main
.extern exit
.extern _post_init
.extern _init
.extern _startup_info

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
	movabsq	$_startup_info, %r8			# store address of _startup_info into %r8 because of -mcmodel=large
	and		%rax, %rbx
	test	%rbx, %rbx
	jnz		1f
	mov		%rsp, (%r8)					# store pointer to HIP
	lea		-ARCH_PAGE_SIZE(%rsp), %rdx	# UTCB is below HIP
	movabsq	$_stack, %rsp				# switch to our stack
	add		$ARCH_STACK_SIZE, %rsp
	sub		$16, %rsp					# leave space for Thread and Pd
	jmp		2f
1:
	mov		%rcx, (%r8)					# store pointer to HIP
	movq	$1, 40(%r8)					# store that we're a child
2:
	mov		%rdx, 8(%r8)			 	# store pointer to UTCB
	mov		%rsp,%rcx
	sub		$ARCH_STACK_SIZE,%rcx
	mov		%rcx, 16(%r8)				# store stack-begin
	mov		%rdi, 24(%r8)				# store cpu
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
