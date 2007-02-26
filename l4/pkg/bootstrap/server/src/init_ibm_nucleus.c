/*****************************************************************************/
/**
 * \brief  Init IBM nucleus
 */
/*****************************************************************************/ 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "startup.h"
#include "init.h"

void 
init_ibm_nucleus(l4_kernel_info_t * l4i)
{
  unsigned i;
  
  // for consistency with Dresden we always enter the kernel debugger...
  l4i->kdebug_config = 0x100; 
  
  /* assertion: the entry point is at the beginning of the kernel info
     page */
  
  assert(0x00001000 <= l4i->sigma0_eip && l4i->sigma0_eip < 0x000a0000);
  /* XXX can't think of more sanity checks that l4i really points to
     the L4 kernel info page :-( */
  
  l4i->reserved1.low = l4i->reserved1.high = 0;
  for (i = 2; i < 4; i++)
    l4i->dedicated[i].low = l4i->dedicated[i].high = 0;
  
  if (mb_mod[0].cmdline)	/* do we have a command line for L4? */
    {
      /* more undocumented stuff: process kernel options */
      if (strstr((char *) mb_mod[0].cmdline, "hercules"))
	printf("IBM's Nucleus does not support hercules as setting\n");
      
      if (strstr((char *) mb_mod[0].cmdline, "nowait"))
	l4i->kdebug_config &= ~0x100;
      
      /* all those serial port related options */
      if (strstr((char *) mb_mod[0].cmdline, " -comport"))
	{
	  unsigned port;
	  const unsigned char port_adr_hi[] = {0x00, 0x3F, 0x2F, 0x3E, 0x2E};
	    
	  port = strtoul((strstr((char *) mb_mod[0].cmdline, " -comport")
			  + strlen(" -comport")), NULL, 10);

	  if (port)
	    {
	      printf("  Setting COM%d as L4KD serial port\n", port);
	      l4i->kdebug_config  |= ((port_adr_hi[port] << 4 | 0x8) << 20) & 
		0xfff00000;
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
	    l4i->kdebug_config |= ((115200/rate) << 16) & 0xf0000;
	  else
      	    printf("  INVALID serial rate\n");
	}
	
      if (strstr((char *) mb_mod[0].cmdline, " -VT"))
	{
	  // set to serial
	  if (strstr((char *) mb_mod[0].cmdline, " -VT+"))
      	    printf("  VT-option does not exist - use comspeed/port instead\n");
	}
	
      
      if (strstr((char *) mb_mod[0].cmdline, "-tracepages"))
	{
	  unsigned tracepages;
	  
	  tracepages = strtoul((strstr((char *) mb_mod[0].cmdline, 
			        " -tracepages")
				+ strlen(" -tracepages")), NULL, 10);
	  if (tracepages)
	    {
	      l4i->kdebug_config |= (tracepages & 0xff);
	      printf("  debugger configured with %d trace pages\n", 
		     tracepages);
	    }
	  else 
	    printf("  invalid tracepage number\n");
	}
      
      if (strstr((char *) mb_mod[0].cmdline, "-lastdebugtask"))
	{
	  unsigned ldtask;
	  
	  ldtask = strtoul((strstr((char *) mb_mod[0].cmdline, 
			    " -lastdebugtask")
			    + strlen(" -lastdebugtask")), NULL, 10);
	  if (ldtask) 
	    {
	      l4i->kdebug_permission |= (ldtask & 0xff);
	      printf("  debugger can be invoked by task 0 to %d",
		     ldtask);
	    }
	  else 
	    printf("  invalid debug task number\n");
	}
      
      if (strstr((char *) mb_mod[0].cmdline, "-d-no-mapping"))
	l4i->kdebug_config &= ~0x100;
      
      if (strstr((char *) mb_mod[0].cmdline, "-d-no-registers"))
	l4i->kdebug_config &= ~0x200;
      
      if (strstr((char *) mb_mod[0].cmdline, "-d-no-memory"))
	l4i->kdebug_config &= ~0x400;
      
      if (strstr((char *) mb_mod[0].cmdline, "-d-no-modif"))
	l4i->kdebug_config &= ~0x800;
      
      if (strstr((char *) mb_mod[0].cmdline, "-d-no-io"))
	l4i->kdebug_config &= ~0x1000;
      
      if (strstr((char *) mb_mod[0].cmdline, "-d-no-protocol"))
	l4i->kdebug_config &= ~0x1000;
    }
  else 
    printf("no configuration string for the nucleus\n");
  
  printf("  kernel-debug config: %x, %x\n", 
	 l4i->kdebug_config, l4i->kdebug_permission);
}

