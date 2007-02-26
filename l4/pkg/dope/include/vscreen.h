/*
 * \brief   Interface of VScreen library
 * \date    2002-11-25
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


/******************************************
 * HANDLE SHARED MEMORY BUFFER OF VSCREEN *
 ******************************************/

/*** RETURN LOCAL ADDRESS OF SPECIFIED SHARED MEMORY BLOCK ***
 *
 * \param smb_ident  identifier string of the shared memory block
 * \return           address of the smb block in local address space
 *
 * The format of the smb_ident string is platform dependent. The
 * result of the VScreen widget's map function can directly be passed
 * into this function. (as done by vscr_get_fb)
 */
extern void *vscr_map_smb(char *smb_ident);



/*************************
 * HANDLE VSCREEN SERVER *
 *************************/

/*** CONNECT TO SPECIFIED VSCREEN WIDGET SERVER ***
 *
 * \param vscr_ident  identifier of the widget's server
 * \return            id to be used for the vscr_server-functions
 */
extern void *vscr_connect_server(char *vscr_ident);


/*** CLOSE CONNECTION TO VSCREEN SERVER ***
 *
 * \param vscr_server_id  id of the vscreen widget server to disconnect from
 */
extern void  vscr_release_server_id(void *vscr_server_id);


/*** WAIT FOR SYNCHRONISATION ***
 *
 * This function waits until the current drawing period of this
 * widget is finished.
 */
extern void  vscr_server_waitsync(void *vscr_server_id);


/*** REFRESH RECTANGULAR WIDGET AREA ASYNCHRONOUSLY ***
 *
 * This function refreshes the specified area of the VScreen widget
 * in a best-efford way. The advantage of this function over a
 * dope_cmd call of vscr.refresh() is its deterministic execution
 * time and its nonblocking way of operation.
 */
extern void  vscr_server_refresh(void *vscr_server_id, int x, int y, int w, int h);



/*****************************
 * VSCREEN UTILITY FUNCTIONS *
 *****************************/


/*** RETURN LOCAL ADDRESS OF THE FRAMEBUFFER OF THE SPECIFIED VSCREEN ***
 *
 * \param app_id     DOpE application id to which the VScreen widget belongs
 * \param vscr_name  name of the VScreen widget
 * \return           address of the VScreen buffer in the local address space
 */
extern void *vscr_get_fb(int app_id, char *vscr_name);


/*** GET SERVER ID OF THE SPECIFIED VSCREEN WIDGET ***
 *
 * \param app_id     DOpE application id to which the VScreen widget belongs
 * \param vscr_name  name of the VScreen widget
 * \return           id to be used for the vscr_server-functions
 */
extern void *vscr_get_server_id(int app_id, char *vscr_name);

