.global start 
.global entry_ex
.global entry_int
.extern main
.extern idt_callbacks

.set MULTIBOOT_PAGE_ALIGN,			1 << 0
.set MULTIBOOT_MEMORY_INFO,			1 << 1
.set MULTIBOOT_HEADER_MAGIC,		0x1BADB002
.set MULTIBOOT_HEADER_FLAGS,		MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
.set MULTIBOOT_CHECKSUM,			-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

.set PAGE_SIZE,						4096
.set STACK_SIZE,					PAGE_SIZE

.macro BUILD_ERR_ISR no
	.global entry_\no
	entry_\no:
	pushl	$\no					# the interrupt-number
	jmp		isr
.endm
.macro BUILD_DEF_ISR no
	.global entry_\no
	entry_\no:
	pushl	$0						# error code
	pushl	$\no					# the interrupt-number
	jmp		isr
.endm

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

BUILD_DEF_ISR 0
BUILD_DEF_ISR 1
BUILD_DEF_ISR 2
BUILD_DEF_ISR 3
BUILD_DEF_ISR 4
BUILD_DEF_ISR 5
BUILD_DEF_ISR 6
BUILD_DEF_ISR 7
BUILD_ERR_ISR 8
BUILD_DEF_ISR 9
BUILD_ERR_ISR 10
BUILD_ERR_ISR 11
BUILD_ERR_ISR 12
BUILD_ERR_ISR 13
BUILD_ERR_ISR 14
BUILD_DEF_ISR 15
BUILD_DEF_ISR 16
BUILD_ERR_ISR 17
BUILD_DEF_ISR 18
BUILD_DEF_ISR 19
BUILD_DEF_ISR 20
BUILD_DEF_ISR 21
BUILD_DEF_ISR 22
BUILD_DEF_ISR 23
BUILD_DEF_ISR 24
BUILD_DEF_ISR 25
BUILD_DEF_ISR 26
BUILD_DEF_ISR 27
BUILD_DEF_ISR 28
BUILD_DEF_ISR 29
BUILD_DEF_ISR 30
BUILD_DEF_ISR 31
BUILD_DEF_ISR 32
BUILD_DEF_ISR 33
BUILD_DEF_ISR 34
BUILD_DEF_ISR 35
BUILD_DEF_ISR 36
BUILD_DEF_ISR 37
BUILD_DEF_ISR 38
BUILD_DEF_ISR 39
BUILD_DEF_ISR 40
BUILD_DEF_ISR 41
BUILD_DEF_ISR 42
BUILD_DEF_ISR 43
BUILD_DEF_ISR 44
BUILD_DEF_ISR 45
BUILD_DEF_ISR 46
BUILD_DEF_ISR 47

isr:
	pusha
	mov		32(%esp),%eax
	mov		$idt_callbacks,%ecx
	mov		(%ecx,%eax,4),%eax
	call	*%eax
	popa
	add		$8,%esp					# remove error-code from stack
	iret

# kernel stack
.align PAGE_SIZE
.rept STACK_SIZE
	.byte	0
.endr
kernelStack:
