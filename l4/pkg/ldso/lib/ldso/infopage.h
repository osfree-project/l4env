/**
 * \file   ldso/lib/ldso/infopage.h
 * \brief  Adaption layer for Linux system calls to L4env
 *
 * \date   2005/12
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef _INFOPAGE_H_
#define _INFOPAGE_H_

#include <l4/env/env.h>

void infopage_show_sections(void);
void infopage_add_mmap_area(void);

extern l4env_infopage_t *global_env;

#endif
