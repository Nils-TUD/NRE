/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

# A fast reply to our client, called by a return to a portal function.
.global portal_reply_landing_spot
portal_reply_landing_spot:
	# w0: NOVA_IPC_REPLY
	mov     $1,     %al
	# keep a pointer to ourself on the stack
	# ecx: stack
	lea     -4(%esp), %ecx
	sysenter
