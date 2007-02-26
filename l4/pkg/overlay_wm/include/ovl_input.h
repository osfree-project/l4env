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
extern int ovl_input_init(char *ovl_name);


/*** REGISTER CALLBACK FUNCTION FOR BUTTON INPUT EVENTS ***
 *
 * \param cb   function that should be called for each incoming event
 * \return     0 on success
 */
extern int ovl_input_button(void (*cb)(int type, int code));


/*** REGISTER CALLBACK FUNCTION FOR MOTION INPUT EVENTS ***
 *
 * \param cb   function that should be called for each motion event
 * \return     0 on success
 */
extern int ovl_input_motion(void (*cb)(int mx, int my));


/*** PROCESS INCOMING INPUT EVENTS ***
 *
 * Normally, this function never returns.
 * When an event is received, the registered callback functions are invoked.
 *
 * \return negative error code or never
 */
extern int ovl_input_eventloop(void);
