/**
 * \file   ldso/lib/ldso/malloc.h
 * \brief  Simple malloc for demangle library.
 *
 * \date   2006/01
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef _MALLOC_H_
#define _MALLOC_H_

#include <stddef.h>

int   demangle_malloc_reset(void);
void *demangle_malloc(size_t size);
void *demangle_calloc(size_t nmemb, size_t size);
void *demangle_realloc(void *ptr, size_t size);
void  demangle_free(void *ptr);

#endif
