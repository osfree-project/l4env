/* $Id$ */
/**
 * \file    rtc/server/src/main.c
 * \brief   Initialization and main server loop
 *
 * \date    09/23/2003
 * \author  Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <stdio.h>

#include <l4/names/libnames.h>
#include <l4/util/rdtsc.h>

#include "rtc-server.h"

char LOG_tag[9] = "rtc";

l4_uint32_t system_time_offs_rel_1970;

void get_base_time(void);

int
l4rtc_if_get_offset_component(CORBA_Object _dice_corba_obj,
			      l4_uint32_t *offset,
		    	      CORBA_Environment *_dice_corba_env)
{
  *offset = system_time_offs_rel_1970;
  return 0;
}

int
l4rtc_if_get_linux_tsc_scaler_component(CORBA_Object _dice_corba_obj,
					l4_uint32_t *scaler,
					CORBA_Environment *_dice_corba_env)
{
  *scaler = l4_scaler_tsc_linux;
  return 0;
}

int
main(int argc, char *argv[])
{
  l4_calibrate_tsc();

  get_base_time();
  names_register("RTC");

  l4rtc_if_server_loop(0);
}

