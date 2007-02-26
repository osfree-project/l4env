/*
 * \brief   Nitpicker screen initialization interface
 * \date    2004-08-24
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef _NITPICKER_SCRINIT_H_
#define _NITPICKER_SCRINIT_H_

extern int    scr_width, scr_height;    /* screen dimensions      */
extern int    scr_depth;                /* color depth            */
extern int    scr_linelength;           /* bytes per scanline     */
extern void  *scr_adr;                  /* physical screen adress */


/*** INITIALIZE SCREEN ***
 *
 * After a successful call of this function, the scr variables are set to
 * valid values and we are ready to scribble into the physical frame buffer
 *
 * \return   0 on success
 */
extern int scr_init(void);

#endif /* _NITPICKER_SCRINIT_H_ */
