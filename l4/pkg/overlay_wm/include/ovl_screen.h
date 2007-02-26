/*
 * \brief   Interface of Overlay Screen library
 * \date    2003-08-18
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */


/*** INIT CONNECTION TO OVERLAY-WM SERVER ***
 *
 * \param ovl_name   name of the overlay server to connect to
 * \return           0 on success
 */
extern int ovl_screen_init(char *ovl_name);


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

