/*!
 * \file	elf64.h
 * \brief	
 *
 * \date	2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the exec package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef __ELF64_H_
#define __ELF64_H_

#include <l4/exec/elf.h>

#include "exc.h"
#include "exc_obj.h"

/** Class for ELF64 objects */
class elf64_obj_t: public exc_obj_t
{
  public:
    elf64_obj_t(exc_img_t *img);
};

int elf64_obj_check_ftype(exc_img_t *img, l4env_infopage_t *env, int verbose);
int elf64_obj_new(exc_img_t *img, exc_obj_t **exc_obj, l4env_infopage_t *env,
		  l4_uint32_t _id);

#endif /* __L4_EXEC_SERVER_ELF64_H */

