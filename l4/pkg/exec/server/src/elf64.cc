/* $Id$ */
/**
 * \file	server/src/elf64.cc
 * \brief	ELF64 exec layer
 *
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the exec package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include <l4/exec/errno.h>

#include "elf64.h"

int
elf64_obj_new(exc_img_t *img, exc_obj_t **exc_obj, l4env_infopage_t *env,
              l4_uint32_t _id)
{
  return -L4_EXEC_BADFORMAT;
}

int
elf64_obj_check_ftype(exc_img_t *img, l4env_infopage_t *env, int verbose)
{
  /* no valid binary found */
  return -L4_EXEC_BADFORMAT;
}

