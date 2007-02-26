/*
 * \brief   Nitpicker server interface
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

#ifndef _NITPICKER_SERVER_H_
#define _NITPICKER_SERVER_H_

#include "nitpicker-server.h"

/*** NITPICKER SERVER ID ***/
extern CORBA_Object nit_server;


/*** COMPARE IF TWO CORBA OBJECTS ARE FROM THE SAME DOMAIN ***/
extern int dice_obj_eq(CORBA_Object o1, CORBA_Object o2);


/*** START PROCESSING CLIENT REQUESTS ***
 *
 * Normally, this function does not return.
 * If there is something wrong with the startup
 * of the server it returns a negative error code.
 * The error code is platform dependent.
 */
extern int start_server(void);


#endif /* _NITPICKER_SERVER_H_ */
