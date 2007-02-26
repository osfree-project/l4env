/* $Id$ */

/*!
 * \file	con/include/l4/con/l4con_pslim.h
 *
 * \brief	con protocol definitions - pSLIM part
 *
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 * These macros are used as parameters for the IDL functions.
 */

#ifndef _L4CON_L4CON_PSLIM_H
#define _L4CON_L4CON_PSLIM_H

/* L4 includes */
#include <l4/con/l4con.h>
#include <l4/sys/types.h>
#include <l4/sys/l4int.h>

/******************************************************************************/
/*!\name Param macros for bmap_*
 *
 * Bitmap type - start least or start most significant bit
 ******************************************************************************/
/*@{*/
#define	pSLIM_BMAP_START_MSB	0x02	/*!<\brief `pbm'-style: "The bits are 
					 * stored eight per byte, high bit first
					 * low bit last." */
#define	pSLIM_BMAP_START_LSB	0x01	/*!< the other way round*/
/*@}*/

/******************************************************************************/
/*!\name param macros for cscs_*
 *
 * YUV types - plane and packed
 ******************************************************************************/
/*@{*/
#define	pSLIM_CSCS_PLN_I420	0x01	/*!< 12 bits/pixel YUV 4:2:0 */
#define pSLIM_CSCS_PLN_YV12	0x02	/*!< 12 bits/pixel YVU 4:2:0 */
#define	pSLIM_CSCS_PCK_YUY2	0x81	/*!< 16 bits/pixel YUYV 4:2:2 */
#define pSLIM_CSCS_PCK_Y211	0x82	/*!<  8 bits/pixel YUV 2:1:1 */
/*@}*/

/******************************************************************************
 * data types
 ******************************************************************************/

/** rectangular area of the virtual framebuffer  */
typedef struct pslim_rect 
{
  l4_int16_t  x, y;	/**< position in vfb (x,y); negative values allowed */
  l4_uint16_t w, h;	/**< geometry (width, height) */
} pslim_rect_t;

/** color type for the virtual framebuffer */
typedef l4_uint32_t pslim_color_t;	/*!< max colors are (theoretical) 2^31 */

#endif /* !_L4CON_L4CON_PSLIM_H */
