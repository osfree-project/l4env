#ifndef __L4_EXEC_SERVER_ELF64_H
#define __L4_EXEC_SERVER_ELF64_H

#include <l4/exec/elf.h>

#include "exc.h"
#include "exc_obj.h"

/** Class for ELF64 objects */
class elf64_obj_t: public exc_obj_t
{
  public:
    elf64_obj_t(exc_img_t *img);
};

int
elf64_obj_new(exc_img_t *img, exc_obj_t **exc_obj, l4env_infopage_t *env,
              l4_uint32_t _id);

#endif /* __L4_EXEC_SERVER_ELF64_H */

