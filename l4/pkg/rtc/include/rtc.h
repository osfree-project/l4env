/* $Id$ */
/**
 * \file	rtc/include/rtc.h
 * \brief	RTC library interface
 * 
 * \date	06/15/2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */


#ifndef L4_RTC_RTC_H
#define L4_RTC_RTC_H

#include <l4/sys/l4int.h>

/**
 * Deliver the numbers of seconds elapsed since 01.01.1970. This value is
 * needed by Linux. */
extern int l4rtc_get_seconds_since_1970(l4_uint32_t *seconds);

/**
 * Deliver the scaler 2^32 / (tsc clocks per usec). This value is needed by
 * Linux. */
extern int l4rtc_get_linux_tsc_scaler(l4_uint32_t *scaler);

#endif

