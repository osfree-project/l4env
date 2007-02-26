/*
 * \brief   L4 specific functions to handle shared memory via dm_phys
 * \date    2003-08-04
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>

/*** L4 INCLUDES ***/
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/util/util.h>
#include <l4/dm_phys/dm_phys.h>

/*** LOCAL INCLUDES ***/
#include "map.h"


/*** UTILITY: CONVERT ASCII HEX NUMBER TO AN UNSIGNED 32BIT VALUE ***/
static unsigned long hex2u32(char *s) {
	int i;
	unsigned long result=0;
	for (i=0;i<8;i++,s++) {
		if (!(*s)) return result;
		result = result*16 + (*s & 0xf);
		if (*s > '9') result += 9;
	}
	return result;
}


/*** L4: MAP SHARED MEMORY BLOCK INTO LOCAL ADDRESS SPACE ***/
void *ovl_screen_get_framebuffer(char *smb_ident) {
	l4dm_dataspace_t ds;
	long smb_size = 0;
	void *fb_adr;
	
	printf("get_framebuffer: smb_ident = %s\n",smb_ident);
	
	ds.manager.lh.low  = hex2u32(smb_ident+7);
	ds.manager.lh.high = hex2u32(smb_ident+16);
	ds.id              = hex2u32(smb_ident+33);
	smb_size           = hex2u32(smb_ident+49);
	
	printf("get_framebuffer: l4rm_attach = %lu\n",
		(long)(l4rm_attach(&ds, smb_size, 0, L4DM_RW, (void *)&fb_adr))
	);
	printf("get_framebuffer: fb_adr = 0x%x\n",(int)fb_adr);
	return fb_adr;
}

