.global start 
.global defaultEx
.global defaultInt
.extern error
.extern main

.set MULTIBOOT_PAGE_ALIGN,			1 << 0
.set MULTIBOOT_MEMORY_INFO,			1 << 1
.set MULTIBOOT_HEADER_MAGIC,		0x1BADB002
.set MULTIBOOT_HEADER_FLAGS,		MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
.set MULTIBOOT_CHECKSUM,			-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

.set PAGE_SIZE,						4096
.set STACK_SIZE,					PAGE_SIZE

.section .text

# Multiboot header
.align 4
	.long	MULTIBOOT_HEADER_MAGIC
	.long	MULTIBOOT_HEADER_FLAGS
	.long	MULTIBOOT_CHECKSUM

# the kernel entry point
start:
	cli
	mov		$kernelStack,%esp
	mov		%esp,%ebp
	xor		%ebp,%ebp

	push	%ebx
	call	main
	
1:									# just to be sure...
	jmp	1b

# for all exceptions we don't care about
defaultEx:
	pusha
	call	error
	popa
	add		$4,%esp					# remove error-code from stack
	iret

# for all exceptions and interrupts we don't care about
defaultInt:
	pusha
	call	error
	popa
	iret

# kernel stack
.align PAGE_SIZE
.rept STACK_SIZE
	.byte	0
.endr
kernelStack:
