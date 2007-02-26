#include "string.h"

#include <l4/libpng/l4png_wrap.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/env/errno.h>

#include "util/presenter_conf.h"
#include "model/presenter.h"
#include "util/module_names.h"
#include "view/display.h"

int init_display(struct presenter_services *p);

#define _DEBUG 0

#define MAX_WIDTH 1024
#define MAX_HEIGHT 1024

static u16 *back_scr, *screen;

static void scale_slide(s32 w, s32 h,s32 img_w, s32 img_h,u16 *src,
                        u16 *scradr,s32 scr_width);

/* CALCULATE 16BIT RAW SCREEN FROM DATASPACE INCLUDING PNG FILE */
static int display_show_slide(char *addr, int addr_buf_size, u16 *scr_adr,
                              s32 img_w, s32 img_h) {
    int width, height,res, row, k;
    s32 offs_w, offs_h;
    u16 *dst, *scr_dst;

    /* backup original screen size and dst address*/
    offs_w = img_w;
    offs_h = img_h;
    scr_dst = scr_adr;

    width = png_get_width(addr, addr_buf_size);

    if (width <= 0) return -1;

    height = png_get_height(addr, addr_buf_size);

    if (height <= 0) return -1;

    /* backup back_screen address */
    dst = back_scr;

    memset(dst,0,sizeof(u16)*MAX_WIDTH*MAX_HEIGHT);

    LOGd(_DEBUG,"width: %d, height: %d, img_w: %ld, img_h: %ld",
        width,height,img_w,img_h);

    /* check if image fits on screen */
    if (img_h < height || img_w < width) {

        res = png_convert_RGB16bit(addr,screen,addr_buf_size,
                                   sizeof(u16)*MAX_WIDTH*MAX_HEIGHT,width);

        // check if something went wrong during read and convert of png file
        if (res != 0) return res;

        /* recalculate size properties of image */
        if (height > width) {
            img_w = width * img_h / height;
            scr_dst+=(offs_w - img_w)>>1;

            /* img_w increased after recalculation over offs_x so we have
            * to calculate img_h */
            if (img_w > offs_w) {
                img_w = offs_w;
                img_h = height * img_w / width;
                scr_dst = scr_adr;
                scr_dst+=(offs_h - img_h)/2*img_w;
            }
        }

        if (width > height) {
            img_h = height * img_w / width;
            scr_dst+=(offs_h - img_h)/2*img_w;

            /* img_h increased after recalculation over offs_h so we have
            * to calculate img_w */
            if (img_h > offs_h) {
                img_h = offs_h;
                img_w = width * img_h/height;
                scr_dst =scr_adr;
                scr_dst+=(offs_w - img_w)>>1;
            }
        }

        scale_slide(img_w,img_h,width,height,screen,dst,offs_w);

        for (row=0;row<img_h;row++) {
            for (k=0;k<offs_w;k++) {
                *(scr_dst+k) = *(dst+k);
            }
            scr_dst+=offs_w;
            dst+=offs_w;
        }

    }
    /* image is smaller than screen */
    else {

        /* calculate centered destination start address */
        dst+= (((img_h-height)/2))*img_w + ((img_w-width)/2);

        res = png_convert_RGB16bit(addr,dst,addr_buf_size,
                                   sizeof(u16)*MAX_WIDTH*MAX_HEIGHT,offs_w);

        if (res != 0)
            return res;

        /* copy background screen content to visible screen */
        memcpy(scr_adr,back_scr,sizeof(u16)*offs_w*offs_h);
    }

    LOGd(_DEBUG,"copy background screen content into visible screen buffer");
    return 0;
}

static int display_check_slide(char *addr, int addr_buf_size) {
    int width, height;

    width = png_get_width(addr, addr_buf_size);

    if (width <= 0) return -1;

    height = png_get_height(addr, addr_buf_size);

    if (height <= 0) return -1;

    return png_convert_RGB16bit(addr,screen,addr_buf_size,
                                sizeof(u16)*MAX_WIDTH*MAX_HEIGHT,width);
}

s32 scale_xbuf[2000];

/*** SCALE 16BIT IMAGE TO 16BIT SCREEN ***/
static void scale_slide(s32 w, s32 h,s32 img_w, s32 img_h,u16 *src,u16 *scradr, s32 scr_width) {
    float mx,my;
    long i,j;
    float sx = 0.0 ,sy = 0.0;
    u16 *s,*d;

    if (w) mx = (float)img_w / (float)w;
    else mx=0.0;

    if (h) my = (float)img_h / (float)h;
    else my=0.0;

    /* calculate x offsets */
    for (i=w;i--;) {
        scale_xbuf[i]=(long)sx;
        sx += mx;
    }

    /* draw scaled image */
    for (j=h;j--;) {
        s=src + ((long)sy*img_w);
        d=scradr;
        for (i=w;i--;) *(d++) = *(s + scale_xbuf[i]);
        sy += my;
        scradr+= scr_width;
    }
}

static struct pres_display_services services = {
    display_show_slide,
    display_check_slide,
};

int init_display(struct presenter_services *p) {
    p->register_module(DISPLAY_MODULE,&services);

    back_scr = (u16 *) l4dm_mem_allocate(sizeof(u16)*MAX_WIDTH*MAX_HEIGHT,0);

    if (back_scr == NULL) {
        LOG("not enough memory to allocate background screen buffer");
        return 0;
    }

    screen = (u16 *) l4dm_mem_allocate(sizeof(u16)*MAX_WIDTH*MAX_HEIGHT,0); 

    if (screen == NULL) {
        LOG("not enough memory to allocate screen buffer");
        return 0;
    }

    return 1;
}
