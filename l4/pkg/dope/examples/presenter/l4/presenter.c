/*
 * \brief   DOpE presenter
 * \date    2002-11-21
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This is a simple image viewer for DOpE. It requests
 * a list of BMP images from Grub's information structures
 * and displays them using the VScreen widget.
 */

#include <stdio.h>

#include <l4/thread/thread.h>
#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/oskit10_l4env/support.h>
#include <l4/oskit10/grub_mb_info.h>
#include <l4/sys/types.h>

#include <l4/dope/dopelib.h>        /* DOpE client lib */
#include <l4/dope/vscreen.h>        /* DOpE VScreen lib */
#include "dope-config.h"

#define MIN(a,b) (a<b?a:b)

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
static struct grub_mod_list *curr_mod;
static struct bmp_struct *curr_bmp;
static char str[256];
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


extern struct grub_multiboot_info *_mbi;

/*** REQUEST BINARY MODULE ***/
static struct grub_mod_list *get_mod_by_index(int idx) {
	idx = idx%_mbi->mods_count;
	return (struct grub_mod_list *)(_mbi->mods_addr + (idx*sizeof(struct grub_mod_list)));	
}


static void display_image(int img_idx) {

	sprintf(str, "t.print(\"display image %d:\\n\")", (int)(img_idx%_mbi->mods_count));
	dope_cmd(app_id, str);

	curr_mod=get_mod_by_index(img_idx);

	sprintf(str, "t.print(\" mod_start= %lu\\n\")", (adr)curr_mod->mod_start);
	dope_cmd(app_id, str);

	sprintf(str, "t.print(\" mod_end  = %lu\\n\")", (adr)curr_mod->mod_end);
	dope_cmd(app_id, str);

	sprintf(str, "t.print(\" mod_size = %lu\\n\")", (adr)curr_mod->mod_end - 
	                                                (adr)curr_mod->mod_start);
	dope_cmd(app_id, str);

	curr_bmp = (struct bmp_struct *)(2+(adr)curr_mod->mod_start);

	sprintf(str, "t.print(\" width    = %d\\n\")", (int)curr_bmp->width);
	dope_cmd(app_id, str);

	sprintf(str, "t.print(\" height   = %d\\n\")", (int)curr_bmp->height);
	dope_cmd(app_id, str);
	
	bmp_to_raw16(curr_bmp, scradr);
	dope_cmd(app_id,"vscr.refresh()");
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


int main(void) {

	LOG_init("presenter");
	OSKit_libc_support_init(500*1024);

	/* init DOpE library */
	dope_init();
	
	/* register DOpE-application */
	app_id = dope_init_app("Fiasco->DOpE->Presenter");
	
	/* create and open window with pSLIM-widget */
	dope_cmd(app_id, "a=new Window()" );
	dope_cmd(app_id, "vscr=new VScreen()" );
	dope_cmd(app_id, "vscr.setmode(1024,768,16)" );
	
	dope_cmd(app_id, "a.set(-x 100 -y 150 -w 320 -h 240 -fitx yes -fity yes -background off -content vscr)" );
	dope_cmd(app_id, "a.open();" );
	dope_cmd(app_id, "tw=new Window()" );
	dope_cmd(app_id, "t=new Terminal()" );
	dope_cmd(app_id, "tw.set(-content t -scrollx yes -scrolly yes)" );
	dope_cmd(app_id, "tw.open()" );
	
	dope_bind(app_id,"vscr","press",  vscr_press_callback,  (void *)0x456);
	
	scradr = vscr_get_fb( dope_cmd(app_id, "vscr.map()") );
	display_image(curr_img_idx);
	
	/* enter mainloop */
	dope_eventloop(app_id);
	return 0;
}
