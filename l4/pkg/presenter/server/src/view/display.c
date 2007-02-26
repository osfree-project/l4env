#include "string.h"

#include "util/presenter_conf.h"
#include "model/presenter.h"
#include "util/module_names.h"
#include "view/display.h"

struct bmp {
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

static s32 scr_w = 1024;
static s32 scr_h = 768;
static struct bmp *curr_bmp;

int init_display(struct presenter_services *p);

static int bmp_to_raw16(struct bmp *bmp,u16 *dst) {
        u8 *src;
        s32 i,j,num_pixels,r,g,b,line_pad;
        s32 m;

        memset(dst,0,scr_w*scr_h*2);

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

static void display_show_slide(char *addr,u16 *scradr) {
   	curr_bmp = (struct bmp *)(2+(unsigned long)addr);
        bmp_to_raw16(curr_bmp, scradr);
}


static struct pres_display_services services = {
	display_show_slide,
};

int init_display(struct presenter_services *p) {
	p->register_module(DISPLAY_MODULE,&services);
	return 1;
}
