/*
 * (c) 2004 Technische Universität Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */


#include <l4/sys/syscalls.h>
#include <l4/crtx/ctor.h>
#include <stdio.h>
#include <l4/util/idt.h>

#define LINUX_SYSCALL 0x80

void trampoline(void);
void int80_warning(unsigned);

static struct {
  l4util_idt_header_t header;
  l4util_idt_desc_t   desc[0x20];
} __attribute__((packed)) idt;

void int80_warning (unsigned error)
{
  if (error == (LINUX_SYSCALL << 3 | 2))
    printf(" => Detected illegal Linux syscall,\n"
	   " => add \"-n%d\" to your Fiasco-UX invocation to allow it!\n",
	   l4_myself().id.task);

  l4util_idt_entry(&idt.header, 0xd, 0);	// kill vector

  enter_kdebug("General Protection Fault (13)");
}

asm ("trampoline:		\n\t"
     "call int80_warning	\n\t"
     "lea 4(%esp), %esp		\n\t"		// pop error code
     "iret			\n\t");

static void setup_idt(void)
{
  l4util_idt_init (&idt.header, 0x20);
  l4util_idt_entry(&idt.header, 0xd, trampoline);
  l4util_idt_load (&idt.header);
}

L4C_CTOR(setup_idt, 10);
