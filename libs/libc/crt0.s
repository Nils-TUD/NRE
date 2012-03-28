.section .text

.global _start
.extern main
.extern exit
.extern _init

_start:
	# mark the beginning of the call-trace
	mov		$0,%ebp

	# call function in .init-section
	call	_init
	# finally, call main
	call	main

	push	%eax
	call	exit
	# just to be sure
	1:		jmp	1b
