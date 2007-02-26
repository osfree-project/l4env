/*
 * \brief   L4 specific DOpE VScreen library
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <stdio.h>
#include <l4/names/libnames.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/util/util.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/dope/vscr-client.h>
#include <l4/dope/vscreen.h>

#define MAX_VSCREENS 32

//static sm_exc_t _ev;
static CORBA_Environment env = dice_default_environment;

struct vscr {
	char            *name;
	l4_threadid_t    tid;
} vscreens[MAX_VSCREENS];



/*** ALLOCATE NEW VSCR ID ***/
static int get_new_index(void) {
	int i;
		
	/* find free vscr id */
	for (i=0;i<MAX_VSCREENS;i++) {
		if (!vscreens[i].name) break;
	}

	if (i>=MAX_VSCREENS) return -1;
	return i;
}


/*** CHECK IF A GIVEN VSCR ID IS VALID ***/
static int valid_index(int id) {

	if ((id<0) || (id>=MAX_VSCREENS)) return 0;
	if (!vscreens[id].name) return 0;
	return 1;
}


static void release_index(int i) {
	
	if (!valid_index(i)) return;
	vscreens[i].name = NULL;    
}


void *vscr_connect_server(char *ident) {
	int i;
	
	if (ident) printf("libVScr(get_id): ident = %s\n",ident);
	if ((i = get_new_index()) <0) return NULL;

	vscreens[i].name = ident;
	
	/* request thread id of VScreen-server using its identifier */
	if (names_waitfor_name(ident, &vscreens[i].tid, 50000) == 0) {
		printf("libVScr(get_id): VScreen-server not found!\n");
		return NULL;
	}

	return (void *)i+1;
}


void vscr_release_server_id(void *id) {
	int i = ((int)id) - 1;
	
	if (!valid_index(i)) return;
	
	/* !!! unmap vscreen memory !!! */

	release_index(i);
}


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


//static char vscr_ident_buf[256];

   
void *vscr_map_smb(char *smb_ident) {
	l4dm_dataspace_t ds;
	long smb_size = 0;
	void *fb_adr;
	
//  	int i = ((int)id) - 1;
//  char *vscr_ident = &vscr_ident_buf[0];
//  	
//  if (!valid_index(i)) return NULL;
//  
//  printf("libVScr(get_fb): dope_vscr_map = %lu\n",(long)
//  	dope_vscr_map_call(&vscreens[i].tid,&vscr_ident,&env)
//  );
//

	printf("libVScreen(get_fb): smb_ident = %s\n",smb_ident);
	
	ds.manager.lh.low  = hex2u32(smb_ident+7);
	ds.manager.lh.high = hex2u32(smb_ident+16);
	ds.id              = hex2u32(smb_ident+33);
	smb_size           = hex2u32(smb_ident+49);
	
	printf("hl.low=%x, lh.high=%x, id=%x\n",
					ds.manager.lh.low,
					ds.manager.lh.high,
					ds.id);
	
////    printf("libVScr(get_fb): vscr_ident = %s\n",vscr_ident);
////    printf("libVScr(get_fb): ds.id = %lu\n",(long)vscreens[i].ds.id);
	printf("libVScr(get_fb): l4rm_attach = %lu\n",
    	(long)(l4rm_attach(&ds, smb_size, 0, L4DM_RW, (void *)&fb_adr))
	);
	printf("libVScr(get_fb): fb_adr = 0x%x\n",(int)fb_adr);
//  return vscreens[i].bufadr;
	return fb_adr;
	
}


void vscr_server_waitsync(void *id) {
	int i = ((int)id) - 1;
	if (!valid_index(i)) return;
	dope_vscr_waitsync_call(&vscreens[i].tid,&env);
	l4_thread_switch(L4_NIL_ID);
//  l4_sleep(10);
}


void vscr_server_refresh(void *id, int x, int y, int w, int h) {
	int i = ((int)id) - 1;
	if (!valid_index(i)) return;
	dope_vscr_refresh_call(&vscreens[i].tid, x, y, w, h, &env);
	l4_thread_switch(L4_NIL_ID);
}
