
#include <l4/sys/syscalls.h>
#include <l4/crtx/ctor.h>
#include <stdio.h>
#include "idt.h"

#define LINUX_SYSCALL 0x80

void trampoline(void);
void int80_warning(unsigned);

static idt_t idt;

void int80_warning (unsigned error)
{
  if (error == (LINUX_SYSCALL << 3 | 2))
    printf(" => Detected illegal Linux syscall,\n"
	   " => add \"-n%d\" to your Fiasco/UX invocation to allow it!\n",
	   l4_myself().id.task);

  MAKE_IDT_DESC(idt, 0xd, 0);			// kill vector

  enter_kdebug("General Protection Fault (13)");
}

asm ("trampoline:		\n\t"
     "call int80_warning	\n\t"
     "lea 4(%esp), %esp		\n\t"		// pop error code
     "iret			\n\t");

static void setup_idt(void)
{
  int i;

  idt.limit = 0x20 * 8 - 1;
  idt.base  = idt.desc;

  for (i = 0; i < 0x20; i++)
    MAKE_IDT_DESC(idt, i, 0);

  MAKE_IDT_DESC(idt, 0xd, trampoline);

  asm volatile ("lidt (%%eax)" : : "a" (&idt));
}

L4C_CTOR(setup_idt, 10);
