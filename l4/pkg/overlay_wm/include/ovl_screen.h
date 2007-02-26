/*
 * \brief   Interface of Overlay Screen library
 * \date    2003-08-18
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the Overlay WM package, which is distributed
 * under the  terms  of the GNU General Public Licence 2. Please see
 * the COPYING file for details.
 */


/*** INIT CONNECTION TO OVERLAY-WM SERVER ***
 *
 * \param ovl_name   name of the overlay server to connect to
 * \return           0 on success
 */
extern int ovl_screen_init(char *ovl_name);


/*** REQUEST WIDTH OF PHYSICAL SCREEN ***/
extern int ovl_get_phys_width(void);


/*** REQUEST HEIGHT OF PHYSICAL SCREEN ***/
extern int ovl_get_phys_height(void);


/*** REQUEST COLOR MODE OF PHYSICAL SCREEN ***/
extern int ovl_get_phys_mode(void);


/***************************
 * OVERLAY SCREEN HANDLING *
 ***************************/

/*** OPEN NEW OVERLAY SCREEN ***
 *
 * \param w      width of screen
 * \param h      height of screen
 * \param depth  color depth
 * \return       0 on success
 */
extern int ovl_screen_open(int w, int h, int depth);


/*** CLOSE OVERLAY SCREEN ***
 *
 * \return  0 on success
 */
extern int ovl_screen_close(void);


/*** MAP FRAME BUFFER OF OVERLAY SCREEN ***
 *
 * \return  pointer to mapped frame buffer
 */
extern void *ovl_screen_map(void);


/*** REFRESH OVERLAY SCREEN AREA ***
 *
 * \param x,y,w,h  dirty screen area
 */
extern void ovl_screen_refresh(int x, int y, int w, int h);

