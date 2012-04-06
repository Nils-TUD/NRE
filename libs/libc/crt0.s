.section .text

.global _start
.extern start
.extern exit
.extern _init

_start:
	mov		%esp, _hip				# store pointer to HIP
	lea		-0x1000(%esp), %edx		# UTCB is below HIP
	mov		$stack, %esp			# switch to our stack
	sub		$4,%esp					# leave space for Ec
	push	%edx					# push UTCB
	push	%eax					# push cpu number

	# call function in .init-section
	call	_init
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

# pointer to HIP
.section .bss.hip
.global _hip
_hip:
	.long	0

.section .bss.stack
.align 0x1000
	.space	0x1000
stack:
