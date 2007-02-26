/**
 * \file	roottask/server/src/bootquota.h
 * \brief	bootquota handling
 *
 * \date	05/10/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author      Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 **/
#ifndef BOOTQUOTA_H
#define BOOTQUOTA_H

#include "types.h"

typedef struct
{
  l4_uint8_t    mcp;
  l4_uint8_t    prio;
  l4_uint8_t    small_space;
  l4_uint8_t    mods;
} __attribute__((packed)) bootquota_t;

void         cfg_bootquota_set(const char *name, const bootquota_t * const b);
void         cfg_bootquota_copy(unsigned i, const char *name);
void         bootquota_init(void);
void         bootquota_set(unsigned i, bootquota_t *b);
bootquota_t* bootquota_get(unsigned n);
void         bootquota_set_default(bootquota_t *b);
int          bootquota_is_default(bootquota_t *b);

#endif
