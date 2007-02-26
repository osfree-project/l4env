/*****************************************************************************/
/**
 * \brief  Init old GMD kernel
 */
/*****************************************************************************/ 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <l4/sys/kernel.h>

#include "exec.h"
#include "startup.h"

#include "init.h"

/*****************************************************************************/
/**
 * \brief Find byte sequence
 */
/*****************************************************************************/ 
static void* 
_scan_kernel(const void * sequence, const int length, const int line)
{
  /* checker */
  unsigned char* find_p;
  void * found_p = 0;
  int found_count = 0;
  for (find_p = (unsigned char *) kernel_low;
      find_p < (unsigned char *) kernel_high;
      find_p++)
    /* check for some code */
    if (memcmp(find_p, sequence, length) == 0)
      { 
	found_p = (void*) find_p; 
	found_count++; 
      };

  switch (found_count)
    {
    case 0:
      printf("  sequence in line %d not found\n", line);
      break;
    case 1:
      break;
    default:
      printf("  sequence in line %d found more than once\n", line);
      found_p = 0;
      break;
    };
  return found_p;
}

#define scan_kernel(sequence) \
_scan_kernel(sequence, sizeof(sequence)-1, __LINE__)


void
init_l4_gmd(l4_kernel_info_t * l4i, exec_info_t * exec_info) 
{
  unsigned i;

  /* assertion: the entry point is at the beginning of the kernel info
     page */
  l4i = (l4_kernel_info_t *) exec_info->entry;
  assert(0x00001000 <= l4i->sigma0_eip && l4i->sigma0_eip < 0x000a0000);
  /* XXX can't think of more sanity checks that l4i really points to
     the L4 kernel info page :-( */
  
  /* XXX undocumented */
  l4i->reserved1.low = l4i->reserved1.high = 0;
  for (i = 2; i < 4; i++)
    l4i->dedicated[i].low = l4i->dedicated[i].high = 0;

  if (mb_mod[0].cmdline) /* do we have a command line for L4? */
    {
      /* more undocumented stuff: process kernel options */
      if (strstr((char *) mb_mod[0].cmdline, "hercules") && have_hercules())
	{
	  unsigned char * kd_init =
	    (unsigned char *)((l4_addr_t)l4i - 0x1000 + l4i->init_default_kdebug);

	  assert(kd_init[0] == 0xb0 && kd_init[1] == 'a');
	  kd_init[1] = 'h';
	}

      if (strstr((char *) mb_mod[0].cmdline, "nowait"))
	{
	  unsigned char * kd_debug;
	  int nowait_ok = 0;

	  for(kd_debug = (unsigned char *) kernel_low;
	      kd_debug < (unsigned char *) kernel_low + (kernel_high - kernel_low);
	      kd_debug++)
	    {
	      /* check for machine code
		 "int $3; jmp 0f; .ascii "debug"; 0:" */
	      if (memcmp(kd_debug, "\314\353\005debug", 8) == 0)
		{
		  kd_debug[0] = 0x90; /* replace "int $3" by "nop" */
		  nowait_ok = 1;
		  break;
		}
	    }
	  assert(nowait_ok);
	}

      /* all those serial port related options */
      {
	unsigned short  portaddr = 0x02F8;	/* This is COM2, the default */
	unsigned short  divisor  = 1;	        /* 115200 bps */
	unsigned char * ratetable = 0;

	if (strstr((char *) mb_mod[0].cmdline, " -comport"))
	  {
	    unsigned port;
	    unsigned char* p_adr;

	    const unsigned char port_adr_hi[] = {0x00, 0x3F, 0x2F, 0x3E, 0x2E};

	    port = strtoul((strstr((char *) mb_mod[0].cmdline, " -comport")
			    + strlen(" -comport")), NULL, 10);

	    if (port)
	      {
		p_adr = scan_kernel("\270\003\207\057\000");
		if (p_adr)
		  {
		    printf("  Setting COM%d as L4KD serial port\n", port);
		    p_adr[3] = port_adr_hi[port];
		    portaddr = (port_adr_hi[port] << 4) | 0x8;
		  }
	      }
	    else
	      printf("  INVALID serial port\n");
	  }

	if (strstr((char *) mb_mod[0].cmdline, " -comspeed"))
	  {
	    unsigned rate;

	    rate = strtoul((strstr((char *) mb_mod[0].cmdline, " -comspeed")
			    + strlen(" -comspeed")), NULL, 10);

	    if (rate)
	      {
		/* check for rate table
		   dw 192, dw 96, dw 48, dw 24, ...
		   the 7th entry is used by default */
		ratetable = scan_kernel("\300\000\140\000\060\000\030\000");
		if (ratetable)
		  {
		    printf("  Setting L4KD serial port rate to %d bps\n", rate);
		    ((short*) ratetable)[7] = 115200/rate;
		    divisor = 115200/rate;
		  }
	      }
	    else
	      printf("  INVALID serial rate\n");
	  }

	if (strstr((char *) mb_mod[0].cmdline, " -VT"))
	  {

	    if (!ratetable)
	      ratetable = scan_kernel("\300\000\140\000\060\000\030\000");

	    if (ratetable)
	      {
		printf("  Enabling serial terminal at %3x, %d bps\n", 
		    portaddr, 115200/divisor);
		/* Uaah, the word before the rate table holds the port address
		 * for remote_outchar */
		((short*) ratetable)[-1] = portaddr;

		/* initialize the serial port */
		asm("mov $0x83,%%al;add $3,%%dx;outb %%al,%%dx;"
		    "sub $3,%%dx;mov %%bx,%%ax;outw %%ax,%%dx;add $3,%%dx;"
		    "mov $0x03,%%al;outb %%al,%%dx;dec %%dx;inb %%dx,%%al;"
		    "add $3,%%dx;inb %%dx,%%al;sub $3,%%dx;inb %%dx,%%al;"
		    "sub $2,%%dx;inb %%dx,%%al;add $2,%%dx;inb %%dx,%%al;"
		    "add $4,%%dx;inb %%dx,%%al;sub $4,%%dx;inb %%dx,%%al;"
		    "dec %%dx;xor %%al,%%al;outb %%al,%%dx;mov $11,%%al;"
		    "add $3,%%dx;outb %%al,%%dx\n"
		    :
		    /* no output */
		    :
		    "d" (portaddr),
		    "b" (divisor)
		    :
		    "eax"
		    );

		if (strstr((char *) mb_mod[0].cmdline, " -VT+"))
		  {
		    unsigned char* p;
		    if ((p = scan_kernel("\203\370\377\165\034\350")))
		      {
			p[-6] = 1;
			printf("  -VT+ mode enabled\n");
		      };
		  }
	      }

	  }

	if (strstr((char *) mb_mod[0].cmdline, " -I+"))
	  {

	    /* idea:
	       in init_timer the idt_gate is set up for the timer irq handler.
	       There we install the kdebug_timer_intr_handler.
	       in kdebug_timer... timer_int is called (saved location of
	       timer_int) */

	    unsigned long timer_int, kdebug_timer_intr_handler;
	    void* real_location;
	    void* tmp_location;
	    unsigned long relocation;

	    unsigned short* new;
	    void* p;
	    void* dest;

	    /* find remote_info_port */
	    tmp_location = scan_kernel("\300\000\140\000\060\000\030\000");
	    if (tmp_location)
	      tmp_location -= 2;

	    /* find reference to remote_info_port */
	    real_location = scan_kernel("\146\201\342\377\017");
	    if (real_location)
	      real_location =
		(void*) ((unsigned long) (((unsigned short*)real_location)[4]));

	    relocation = tmp_location - real_location;

	    /* find the place where the timer_int is set up */
	    p = scan_kernel("\000\000"	/* this is a bit illegal !!!*/
			    "\303"		/* c3		*/
			    "\263\050"	/* b3 28	*/
			    "\267\000"	/* b7 00	*/
			    "\270"		/* b8		*/
			    );
	    if (p)
	      {
		timer_int = ((unsigned long*)p)[2];

		/* find kdebug_timer_intr_handler */
		dest = scan_kernel("\152\200\140\036\152\010"
				   "\037\314\074\014\074\033");
		/* no more comments */
		if (dest)
		  {
		    kdebug_timer_intr_handler =
		      (((unsigned long) dest) - relocation);
		    ((unsigned long*)p)[2] = kdebug_timer_intr_handler;
		    dest = scan_kernel("\015\260\055\314"
				       );
		    if (dest)
		      {
			new = (unsigned short*) ((((unsigned short*)dest)[-5]) + relocation);
			*new = timer_int;
			printf("  I+ enabled\n");
		      };
		  };
	      };
	  };
      }

      /*
	This option disables the patch that eliminates the miscalculation
	of string offsets in messages containing indirect strings

	The patch changes calculation of the first string's offset in the
	message buffer (in ipc_strings).
	This offset was calculated

	addr(msgbuffer) + offset(dword_2) + 4*number_of_dwords

	this is changed to
	addr(msgbuffer) + offset(dword_0) + 4*number_of_dwords
	^^^^^^^
      */
      if (strstr((char *) mb_mod[0].cmdline, " -disablestringipcpatch"))
	printf("  string IPC patch disabled\n");
      else
	{
	  unsigned char * patchme;
	  patchme = scan_kernel("\024\213\126\004\301\352\015\215\164\226\024");
	  if (patchme)
	    {
	      patchme[0] = 0x0c;
	      patchme[10] = 0x0c;
	      printf("  string IPC patch applied\n");
	    }
	  else
	    printf("  string IPC patch not applied - signature not found\n");
	};

      /* heavy undocumented stuff, we simply patch the kernel to get
	 access to irq0 and to be able to use the high resolution timer */
      if (strstr((char *) mb_mod[0].cmdline, "irq0"))
	{
	  unsigned char *kd_debug;
	  int irq0_ok = 0;

	  for(kd_debug = (unsigned char *) kernel_low;
	      kd_debug < (unsigned char *) kernel_low + (kernel_high - kernel_low);
	      kd_debug++)
	    {
	      /* check for byte sequence
		 0xb8 0x01 0x01 0x00 0x00 0xe8 ?? ?? ?? ?? 0x61 0xc3 */
	      if (memcmp(kd_debug, "\270\001\001\000\000\350", 6) == 0)
		{
		  if ((kd_debug[10] == 0141) && (kd_debug[11] == 0303))
		    {
		      kd_debug[1] = '\0'; /* enable irq0 */
		      irq0_ok = 1;
		      break;
		    }
		}
	    }
	  assert(irq0_ok);
	}

      /* boot L4 in real-mode (for old versions) */
      if (strstr((char *) mb_mod[0].cmdline, "boothack"))
	{
	  /* find "cmpl $0x2badb002,%eax" */
	  l4_addr_t dest
	    = (l4_addr_t) scan_kernel("\075\002\260\255\053");

	  if (dest)
	    exec_info->entry = dest;
	}
    }
}

