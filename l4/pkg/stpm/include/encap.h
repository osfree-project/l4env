/*
 * \brief   Macro to instantiate IDL client stubs.
 * \date    2004-11-12
 * \author  Bernhard Kauer <kauer@tudos.org>
 */
/*
 * Copyright (C) 2004  Bernhard Kauer <kauer@tudos.org>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef  _ENCAP_H
#define  _ENCAP_H

#include <l4/names/libnames.h>
#include <l4/env/errno.h>
#include <l4/sys/consts.h>

#define ENCAP_FUNCTION(SERVERNAME,NAME, PARAMS, ...)	  			\
int SERVERNAME##_##NAME PARAMS							\
{	       								        \
	DICE_DECLARE_ENV(env);							\
										\
	if (check_server()) return -L4_EINVAL;					\
	return SERVERNAME##if_##NAME##_call(&server_id , ##__VA_ARGS__ , &env); \
}



#define CHECK_SERVER(NAME) 				\
static l4_threadid_t server_id = L4_INVALID_ID;		\
static int check_server(void)				\
{							\
  if (l4_is_invalid_id(server_id))			\
    {							\
      if (!names_waitfor_name(NAME,&server_id,10000))	\
	return 1;					\
    }							\
  return 0;						\
}

#endif
