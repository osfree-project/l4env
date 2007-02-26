/**
 * \file	roottask/server/src/pager.h
 * \brief	page fault handling
 *
 * \date	05/10/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author      Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 **/
#ifndef PAGER_H
#define PAGER_H

typedef struct {
  l4_umword_t pfa;
  l4_umword_t eip;
} __attribute__((packed)) last_pf_t;

void pager_init(void);
void reset_pagefault(int task);
void pager(void) L4_NORETURN;

extern int no_pentium;		/* 4MB support? */
extern int have_io;		/* Did we receive _any_ IO port from sigma0? */

#endif
