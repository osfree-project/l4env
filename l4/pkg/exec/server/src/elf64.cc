/* $Id$ */
/**
 * \file	server/src/elf64.cc
 * \brief	ELF64 exec layer
 *
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 */
#include <l4/exec/errno.h>

#include "elf64.h"

int
elf64_obj_new(exc_img_t *img, exc_obj_t **exc_obj, l4env_infopage_t *env,
              l4_uint32_t _id)
{
  return -L4_EXEC_BADFORMAT;
}

