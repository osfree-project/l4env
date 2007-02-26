/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the Overlay WM package, which is distributed
 * under the  terms  of the GNU General Public Licence 2. Please see
 * the COPYING file for details.
 */

/*** CALLBACK FOR PRESS EVENTS ***/
extern void ovl_input_press_callback(dope_event *e, void *arg);

/*** CALLBACK FOR RELEASE EVENTS ***/
extern void ovl_input_release_callback(dope_event *e, void *arg);

/*** CALLBACK FOR MOTION EVENTS ***/
extern void ovl_input_motion_callback(dope_event *e, void *arg);
