/**
 * \brief   Support for multiple instances of VERNER
 * \date    2004-02-15
 * \author  Carsten Rietzschel <cr7@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2004  Carsten Rietzschel  <cr7@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*
 *  This file is stolen from GPL'ed DOpE.
 */

#include <l4/env/errno.h>
#include <l4/util/rdtsc.h>
#include <l4/util/macros.h>
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>

#include <stdio.h>

#include "minstances.h"

/* configuration */
#include "verner_config.h"

#define MAX_INSTANCES 10

/*
 * register at nameserver returns instance number 
 * return <0 on error, >=0 is instance number
 */
int register_instance(char* name)
{
  char newname[NAMES_MAX_NAME_LEN];
  int instance_no = 0;
  
  /* create first name to register */
  snprintf(newname,NAMES_MAX_NAME_LEN,"%s%i",name,instance_no);
  /* try to register */
  while (!names_register (newname))
  {
    LOGdL (DEBUG_MINSTANCES,"failed to register \"%s\" at namesserver - next try", newname);
    instance_no++;
    if(instance_no >= MAX_INSTANCES) 
    {
    	Panic("failed to register. really.");
	return -L4_ENOTSUPP;
    }
    /* create next name to register */
    snprintf(newname,NAMES_MAX_NAME_LEN,"%s%i",name,instance_no);
  }
  /* success */
  return instance_no;
}


/*****************************************************************************/
/**
 * \brief Request thread id of registered component at nameserver
 * 
 * \param name name of component
 * \praam instance_no Number of searched instance
 * \param timeout timout in milliseconds
 */
/*****************************************************************************/
l4_threadid_t
find_instance(char* name, int instance_no, int timeout)
{  
  l4_threadid_t thread_id = L4_INVALID_ID;
  char newname[NAMES_MAX_NAME_LEN];
  
  /* create name to check at nameserver */
  snprintf(newname,NAMES_MAX_NAME_LEN,"%s%i",name,instance_no);
  if (!names_waitfor_name (newname, &thread_id, timeout))
  {
    LOGdL (DEBUG_MINSTANCES,"didn't found \"%s\" at namesserver.", newname);
    return L4_INVALID_ID;
  }
  /* found */
  return thread_id;
}
