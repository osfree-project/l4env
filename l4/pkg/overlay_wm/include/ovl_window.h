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
extern int ovl_window_init(char *ovl_name);


/***************************
 * OVERLAY WINDOW HANDLING *
 ***************************/

/*** CREATE NEW WINDOW FOR OVERLAY SCREEN ***
 *
 * \return  new overlay window id
 */
extern int ovl_window_create(void);


/*** DESTROY OVERLAY WINDOW ***
 *
 * \param win_id  id of overlay window to destroy
 */
extern void ovl_window_destroy(int win_id);


/*** OPEN OVERLAY WINDOW ***
 *
 * \param win_id  id of overlay window to open
 */
extern void ovl_window_open(int win_id);


/*** CLOSE OVERLAY WINDOW ***
 *
 * \param win_id  id of overlay window to close
 */
extern void ovl_window_close(int win_id);


/*** SET THE SIZE AND POSITION OF AN OVERLAY WINDOW ***
 *
 * \param win_id   id of overlay window to reposition
 * \param x,y,w,h  new position and size of the window
 */
extern void ovl_window_place(int win_id, int x, int y, int w, int h);


/*** TOP THE SPECIFIED OVERLAY WINDOW ***
 *
 * \param win_id  id of overlay window to top
 */
extern void ovl_window_top(int win_id);


/*************************
 * WINDOW EVENT HANDLING *
 *************************/

/*** REGISTER CALLBACK FOR WINDOW PLACEMENT CHANGES ***
 *
 * \param cb      callback function taking the window id and the new
 *                window position and size as arguments (x,y,w,h)
 * \return        0 on success
 */
extern int ovl_winpos_callback(void (*cb)(int,int,int,int,int));


/*** REGISTER CALLBACK FOR WINDOW CLOSE EVENTS ***
 *
 * \param cb      callback function taking the window id as argument
 * \return        0 on success
 */
extern int ovl_winclose_callback(void (*cb)(int));


/*** REGISTER CALLBACK FOR WINDOW TOP EVENTS ***
 *
 * \param cb      callback function taking the window id as argument
 * \return        0 on success
 */
extern int ovl_wintop_callback(void (*cb)(int));


/*** ATTACH PRIVATE DATA TO AN OVERLAY WINDOW ID ***/
extern void ovl_window_set_private(int win_id, void *private);


/*** REQUEST PRIVATE DATA OF AN OVERLAY WINDOW ID ***/
extern void *ovl_window_get_private(int win_id);
