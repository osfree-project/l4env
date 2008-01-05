/**
 * \file	roottask/server/src/task.h
 * \brief	task resource handling
 *
 * \date	05/10/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author      Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 **/
#ifndef TASK_H
#define TASK_H

#include "types.h"

void    task_init(void);
void    task_set(unsigned begin, unsigned end, int state);
int     task_alloc(unsigned taskno, owner_t owner, int allow_realloc);
int     task_free(unsigned taskno, owner_t owner);
owner_t task_owner(unsigned taskno);
void    task_dump(void);
int     task_next(owner_t owner);
int     task_next_explicit(owner_t owner, unsigned long taskno);
void    task_free_owned(owner_t owner);

#endif
