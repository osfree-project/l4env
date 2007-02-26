/**
 * \file	roottask/server/src/trampoline.h
 * \brief	provides entry function for new task
 *
 * \date	05/10/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author      Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 **/
#ifndef TRAMPOLINE_H
#define TRAMPOLINE_H

#include "types.h"

#define task_trampoline_end ((void *) &_task_trampoline_end)

extern char _task_trampoline_end; /* trampoline.c exec.h */

inline void
task_trampoline(l4_addr_t entry, void *mbi);

#endif
