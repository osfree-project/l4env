#include <stdio.h>
#include <flux/x86/pio.h>


#define enter_kdebug(text) \
asm(\
    "int	$3	\n\t"\
    "jmp	1f	\n\t"\
    ".ascii	\""text "\"\n\t"\
    "1:			\n\t"\
    )

static void
my_pc_reset(void)
{
  outb_p(0x70, 0x80);
  inb_p (0x71);

  while (inb(0x64) & 0x02)
    ;                                                                     

  outb_p(0x70, 0x8F);
  outb_p(0x71, 0x00);

  outb_p(0x64, 0xFE);

  for (;;)
    ;
}

void 
_exit(int __attribute__((unused)) status)
{
  char c;

  printf("\nReturn reboots, \"k\" enters L4 kernel debugger...\n");

  c = getchar();

  if (c == 'k' || c == 'K') 
    enter_kdebug("before reboot");

  my_pc_reset();
}
