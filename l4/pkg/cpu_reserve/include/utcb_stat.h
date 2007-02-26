/*!
 * \file   cpu_reserve/include/utcb_stat.h
 * \brief  Timeslice watching using the UTCB Ring-buffer of scheduling events
 *
 * \date   01/12/2005
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __CPU_RESERVE_INCLUDE_UTCB_STAT_H_
#define __CPU_RESERVE_INCLUDE_UTCB_STAT_H_
#include <l4/sys/utcb.h>

typedef struct {
    unsigned long long type:2;
    unsigned long long id:6;
    unsigned long long release:56;
    unsigned long long left:32;
} l4cpu_reserve_utcb_elem_t;

#define L4CPU_RESERVE_UTCB_STAT_COUNT 30
typedef struct{
    l4_utcb_t utcb;
    l4_uint16_t head, tail, full, dummy;
    l4_uint64_t release;
    l4cpu_reserve_utcb_elem_t stat[L4CPU_RESERVE_UTCB_STAT_COUNT];
} l4cpu_reserve_utcb_t;

typedef enum {
    L4_UTCB_PERIOD_OVERRUN = 0,
    L4_UTCB_RESERVATION_OVERRUN = 1,
    L4_UTCB_NEXT_PERIOD = 2,
    L4_UTCB_NEXT_RESERVATION = 3,
  } l4cpu_reserve_utcb_stat_type;

static inline const char*l4cpu_reserve_utcb_stat_itoa(int type){
     switch(type){
    case 0: return "period overrun";
    case 1: return "reservation overrun";
    case 2: return "next period";
    case 3: return "next reservation";
    }
    return 0;
}


#endif
