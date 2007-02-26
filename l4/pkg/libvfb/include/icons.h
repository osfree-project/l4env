#ifndef __LIBVFB_INCLUDE_ICONS_H_
#define __LIBVFB_INCLUDE_ICONS_H_

#define VFB_ICONSIZE 13

typedef struct vfb_icon_s
{
    unsigned int    width;
    unsigned int    height;
    unsigned int    bytes_per_pixel; /* 3:RGB, 4:RGBA */ 
    unsigned char * pixel_data;
} vfb_icon_t;

const vfb_icon_t * vfb_icon_get_for_name(char * name);
void vfb_icon_get_pixel(const vfb_icon_t * icon, int x, int y,
                        int *r, int *g, int *b, int *a);

#endif
