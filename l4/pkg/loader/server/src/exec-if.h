/*!
 * \file	loader/server/src/exec-if.h
 * \brief	interface to exec layer
 *
 * \date	2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef __EXEC_IF_H_
#define __EXEC_IF_H_

int exec_if_init(void);
int exec_if_get_symbols(app_t *app);
int exec_if_get_lines(app_t *app);
int exec_if_link(app_t *app);
int exec_if_open(app_t *app, const char *fname, l4dm_dataspace_t *ds, 
		 int open_flags);
int exec_if_close(app_t *app);
int exec_if_ftype(l4dm_dataspace_t *ds, l4env_infopage_t *env);

#endif
