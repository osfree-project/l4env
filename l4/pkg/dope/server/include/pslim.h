/*
 * \brief   Interface of DOpE pSLIM widget module
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

struct pslim_methods;
struct pslim_data;

#define PSLIM struct pslim

#ifndef __TYPEDEF_PSLIM_RECT_T__
#define __TYPEDEF_PSLIM_RECT_T__
typedef struct pslim_rect_t {
  s16 x;
  s16 y;
  u16 w;
  u16 h;
} pslim_rect_t;
#endif

#ifndef __TYPEDEF_PSLIM_COLOR_T__
#define __TYPEDEF_PSLIM_COLOR_T__
typedef u32 pslim_color_t;
#endif

#if !defined(strdope_t)
typedef struct {
  u32 snd_size;
  u32 snd_str;
  u32 rcv_size;
  u32 rcv_str;
} strdope_t;
#endif

#define pSLIM_BMAP_START_MSB    0x02
#define pSLIM_BMAP_START_LSB    0x01
#define pSLIM_CSCS_PLN_420      0x12
#define pSLIM_CSCS_PLN_422      0x0a
#define pSLIM_CSCS_PLN_444      0x09
#define pSLIM_CSCS_PCK_411      0x84

#define pSLIM_FONT_CHAR_W       8
#define pSLIM_FONT_CHAR_H       12

struct pslim {
	struct widget_methods   *gen;
	struct pslim_methods    *pslim;
	struct widget_data      *wd;
	struct pslim_data       *pd;
};

struct pslim_methods {
	void (*reg_server) (PSLIM *,char *server_ident);
	u8  *(*get_server) (PSLIM *);

	s32  (*probe_mode) (PSLIM *,s32 width, s32 height, s32 depth);
	s32  (*set_mode)   (PSLIM *,s32 width, s32 height, s32 depth);

	s32  (*fill) (PSLIM *,const pslim_rect_t *,pslim_color_t);
	s32  (*copy) (PSLIM *,const pslim_rect_t *,s32 dx,s32 dy);
	s32  (*bmap) (PSLIM *,const pslim_rect_t *,pslim_color_t fg,pslim_color_t bg,void *bmap,u8 type);
	s32  (*set)  (PSLIM *,const pslim_rect_t *,void *pmap);
	s32  (*cscs) (PSLIM *,const pslim_rect_t *,void *yuv,s8 yuv_type,u8 scale);

	s32  (*puts)      (PSLIM *,const char *s,s16 x,s16 y,pslim_color_t fg,pslim_color_t bg);
	s32  (*puts_attr) (PSLIM *,const char *s,s16 x,s16 y);
};

struct pslim_services {
	PSLIM *(*create) (void);
};
