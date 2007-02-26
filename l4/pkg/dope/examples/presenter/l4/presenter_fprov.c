/*
 * \brief   DOpE presenter using the generic file provider interface
 * \date    2002-11-21
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 * \author  Jork Loeser <jork@os.inf.tu-dresden.de>
 *
 * This is a simple image viewer for DOpE. It requests BMP images from
 * a generic file provider and displays them using the VScreen widget.
 */

/*
 * Copyright (C) 2002-2003
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <stdio.h>
#include <string.h>

#include <l4/thread/thread.h>
#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/sys/types.h>
#include <l4/generic_fprov/generic_fprov-client.h>
#include <l4/util/parse_cmd.h>
#include <l4/sys/types.h>
#include <l4/env/env.h>
#include <l4/env/errno.h>
#include <l4/l4rm/l4rm.h>

#include <l4/dope/dopelib.h>        /* DOpE client lib */
#include <l4/dope/vscreen.h>        /* DOpE VScreen lib */
#include "dopestd.h"

#define MIN(a,b) (a<b?a:b)

char LOG_tag[9] = "fp-pres";
l4_ssize_t l4libc_heapsize = 500*1024;
l4_threadid_t img_server;
int   img_count;
char *img_basename;
char *img_server_name;

struct bmp_struct {
	s32 file_size;
	s32 reserved1;
	s32 data_offset;
	
	s32 info_header_size;           /* info header */
	s32 width;
	s32 height;
	u32 depth;
	s32 compression;
	s32 img_size;
	s32 x_pix_per_m;
	s32 y_pix_per_m;
	s32 num_used_colors;
	s32 num_important_colors;
};


static int curr_img_idx=0;
static s32 scr_w = 1024;
static s32 scr_h = 768;
static u16 *scradr;
static struct bmp_struct *curr_bmp;
static long app_id;                 /* DOpE application id */


/*** CONVERT 24BIT BMP IMAGE TO 16BIT PIXEL DATA ***/
static int bmp_to_raw16(struct bmp_struct *bmp,u16 *dst) {
	u8 *src;
	s32 i,j,num_pixels,r,g,b,line_pad;
	s32 m;

	memset(scradr,0,scr_w*scr_h*2);
	
	if ((bmp->width>scr_w) || (bmp->height>scr_h)) return 0;
	
	src = (u8 *)((adr)bmp + bmp->data_offset);
	num_pixels = bmp->width * bmp->height;
	
	m= (bmp->width*3) & 0x3;
	line_pad = 0;
	if (m == 3) line_pad = 1;
	if (m == 2) line_pad = 2;
	if (m == 1) line_pad = 3;
	
	/* calculate centered destination start address */
	dst+= (bmp->height - 1 + ((scr_h-bmp->height)/2))*scr_w +
	      ((scr_w-bmp->width)/2);
	
	for (i=0;i<bmp->height;i++) {
		for (j=0;j<bmp->width;j++) {
			r= *(src++);
			b= *(src++);
			g= *(src++);
			*(dst+j) = ((r&0xf8)<<8) + ((g&0xfc)<<3) + ((b&0xf8)>>3);
		}
		dst-=scr_w;
		src+=line_pad;
	}
	return 0;
}


static int get_ds_by_index(int idx, char** addr, l4_size_t *size,
                           l4dm_dataspace_t *ds) {
	char buffer[250];
	int  err;
	CORBA_Environment _env = dice_default_environment;
	l4_threadid_t dm_id = l4env_get_default_dsm();

	idx = idx%img_count;
	snprintf(buffer, sizeof(buffer)-1, img_basename, idx);
	buffer[sizeof(buffer)-1]=0;
	err = l4fprov_file_open_call(&img_server, buffer, &dm_id, 0, ds, size, &_env);
	if (err) {
		dope_cmdf(app_id, "t.print(\"Opening file \\\"%s\\\" at %s: %s\\n\")",
		          buffer, img_server_name, l4env_errstr(err));
		return 1;
	}
	err = l4rm_attach(ds, *size, 0, L4DM_RO | L4RM_MAP, (void**)addr);
	if (err) {
		dope_cmdf(app_id, "t.print(\"Error attaching dataspace: %s\\n\")",
		          l4env_errstr(err));
		l4dm_close(ds);
	}
	return err;
}


static void display_image(int img_idx) {
	l4dm_dataspace_t ds;
	char *addr;
	l4_size_t size;
	int err;
	img_idx = ((unsigned int)img_idx)%img_count;
	
	dope_cmdf(app_id, "t.print(\"display image %d:\\n\")", (int)img_idx);

	if (get_ds_by_index(img_idx, &addr, &size, &ds)) return;

	dope_cmdf(app_id, "t.print(\" mod_start= 0x%08x\\n\")", (int)addr);
	dope_cmdf(app_id, "t.print(\" mod_end  = 0x%08x\\n\")", (int)addr + (int)size);
	dope_cmdf(app_id, "t.print(\" mod_size = %lu\\n\")", (int)size);

	curr_bmp = (struct bmp_struct *)(2+(unsigned long)addr);

	dope_cmdf(app_id, "t.print(\" width    = %d\\n\")", (int)curr_bmp->width);
	dope_cmdf(app_id, "t.print(\" height   = %d\\n\")", (int)curr_bmp->height);
	
	bmp_to_raw16(curr_bmp, scradr);
	dope_cmd(app_id,"vscr.refresh()");

	if ((err=l4rm_detach(addr)) !=0) {
		dope_cmdf(app_id, "t.print(\"l4rm_detach(): %s\\n\")", l4env_errstr(err));
	}
	if ((err = l4dm_close(&ds)) != 0) {
		dope_cmdf(app_id, "t.print(\"l4dm_close(): %s\\n\")", l4env_errstr(err));
	}
}


static void vscr_press_callback(dope_event *e,void *arg) {
	char curr_ascii;
	if (e->type == EVENT_TYPE_PRESS) {
		curr_ascii = dope_get_ascii(app_id, e->press.code);

		switch (e->press.code) {
		case 105: /* left arrow */
			curr_img_idx--;
			display_image(curr_img_idx);
			break;
		case 106: /* right arrow */
			curr_img_idx++;
			display_image(curr_img_idx);
			break;
		}
		
		switch (curr_ascii) {
		case 'f':
			dope_cmd(app_id, "a.set(-x -5 -y -22 -w 1034 -h 795)" );
			break;
			
		case 'g':
			dope_cmd(app_id, "a.set(-x 200 -y 100 -w 320 -h 240)" );
			break;
		}
	}
}


int main(int argc, const char**argv) {
	
	if ((parse_cmdline(&argc, &argv,
	     's',"server","file provider name",
	     PARSE_CMD_STRING, "hostfs", &img_server_name,
	     'n',"name","img file basename (base-%d.bmp)",
	     PARSE_CMD_STRING, "img-%d.bmp", &img_basename,
	     'c',"count","img count",
	     PARSE_CMD_INT, 1, &img_count,
	     0)) != 0) return 1;
	
	if (!names_waitfor_name(img_server_name, &img_server, 10000)) {
		printf("Server \"%s\" not found\n", img_server_name);
		return 1;
	}

	/* init DOpE library */
	dope_init();
	
	/* register DOpE-application */
	app_id = dope_init_app("Fiasco->DOpE->Presenter");
	
	/* create and open window with pSLIM-widget */
	dope_cmd(app_id, "a=new Window()" );
	dope_cmd(app_id, "vscr=new VScreen()" );
	dope_cmd(app_id, "vscr.setmode(1024,768,\"RGB16\")" );
	
	dope_cmd(app_id, "a.set(-x 100 -y 150 -w 320 -h 240 -fitx yes -fity yes -background off -content vscr)" );
	dope_cmd(app_id, "a.open();" );
	dope_cmd(app_id, "tw=new Window()" );
	dope_cmd(app_id, "t=new Terminal()" );
	dope_cmd(app_id, "tw.set(-content t -scrollx yes -scrolly yes)" );
	dope_cmd(app_id, "tw.open()" );
	
	dope_bind(app_id,"vscr","press",  vscr_press_callback,  (void *)0x456);
	
	scradr = vscr_get_fb(app_id, "vscr");
	if (!scradr) {
		LOG("Could not map vscreen!\n");
		return 1;
	}

	display_image(curr_img_idx);
	
	/* enter mainloop */
	dope_eventloop(app_id);
	return 0;
}
