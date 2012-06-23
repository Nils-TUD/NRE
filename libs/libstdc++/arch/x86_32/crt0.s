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
#   eax: cpu
#   esp: pointer to Hip

# initial state for non-root:
#   eax: (1 << 31) | cpu
#   esp: stack-pointer
#   ecx: pointer to Hip
#   edx: pointer to Utcb
#   esi: 0 to call main, otherwise the address of the function to call

_start:
	mov		%eax, %ebx
	and		$0x7FFFFFFF, %eax			# remove bit
	and		$0x80000000, %ebx			# the bit says that we're not the root-task
	test	%ebx, %ebx
	jnz		1f
	mov		%esp, _startup_info			# store pointer to HIP
	lea		-0x1000(%esp), %edx		# UTCB is below HIP
	mov		$_stack, %esp				# switch to our stack
	add		$0x1000, %esp
	sub		$8, %esp					# leave space for Ec and Pd
	jmp		2f
1:
	mov		%ecx, _startup_info			# store pointer to HIP
	movl	$1, _startup_info + 20		# store that we're a child
2:
	mov		%edx, _startup_info + 4 	# store pointer to UTCB
	mov		%esp,%ecx
	sub		$0x1000,%ecx
	mov		%ecx, _startup_info + 8		# store stack-begin
	mov		%eax, _startup_info + 12	# store cpu

	# call function in .init-section
	call	_init
	call	_post_init

	# test whether we're root
	test	%ebx, %ebx
	jnz		1f
	# yes, so call main
	call	main
1:
	# check whether we should call a different function
	test	%esi,%esi
	jnz		2f
	call	main
	jmp		3f
2:
	call	*%esi

3:
	# exit
	push	%eax
	call	exit
	# just to be sure
	1:		jmp	1b

# information for startup
.section .bss.startup_info
.global _startup_info
_startup_info:
	.long	0	# HIP
	.long	0	# UTCB
	.long	0	# stack
	.long	0	# cpu
	.long	0	# inited
	.long	0	# child
