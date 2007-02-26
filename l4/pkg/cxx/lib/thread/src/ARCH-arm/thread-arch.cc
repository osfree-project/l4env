/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/cxx/thread.h>
#include <l4/cxx/base.h>

void L4_cxx_start(void);

void L4_cxx_start(void)
{
  asm volatile (".global L4_Thread_start_cxx_thread \n"
                "L4_Thread_start_cxx_thread:        \n"
                "ldmib sp!, {r0}                    \n"
                "ldr pc,1f                          \n"
                "1: .word L4_Thread_execute         \n");
}

void L4_cxx_kill(void)
{
  asm volatile (".global L4_Thread_kill_cxx_thread \n"
                "L4_Thread_kill_cxx_thread:        \n"
                "ldmib sp!, {r0}                   \n"
                "ldr pc,1f                         \n"
                "1: .word L4_Thread_shutdown       \n");
}

