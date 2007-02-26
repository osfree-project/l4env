/**
 * \file	roottask/server/src/quota.h
 * \brief	quota definition for tasks
 *
 * \date	05/10/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author      Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 **/
#ifndef QUOTA_H
#define QUOTA_H

#include <l4/sys/types.h>
#include "types.h"

typedef struct
{
  l4_addr_t low, high;
  l4_addr_t max, current;
} __attribute__((packed)) addr_quota_t;

typedef struct
{
  l4_uint16_t low, high;
  l4_uint16_t max, current;
} __attribute__((packed)) u16_quota_t;

typedef struct
{
  l4_uint8_t  low, high;
  l4_uint8_t  max, current;
} __attribute__((packed)) u8_quota_t;

typedef struct
{
  l4_uint32_t max, current;
} __attribute__((packed)) mask_quota_t;

typedef struct
{
  addr_quota_t  mem, himem;
  u16_quota_t   task;
  u8_quota_t    small;
  l4_uint32_t   irq;
  l4_uint16_t   log_mcp;
  l4_uint16_t   allow_cli;
  l4_int32_t	offset;
} __attribute__((packed)) quota_t;

const char * cfg_quota_set(const char *name, const quota_t * const q);
void     cfg_quota_copy(unsigned i, const char *name);
void     quota_init(void);
void     quota_init_log_mcp(int log_mcp);
void     quota_init_mcp(int mcp);
void     quota_init_prio(int prio);
void     quota_set(unsigned i, const quota_t * const q);
void     quota_reset(owner_t task);
int      quota_check_mem(owner_t t, l4_addr_t where, unsigned amount);
int      quota_check_allow_cli(owner_t t);
int      quota_alloc_mem(owner_t t, l4_addr_t where, unsigned amount);
int      quota_alloc_task(owner_t t, l4_addr_t where);
int      quota_alloc_small(owner_t t, l4_addr_t where);
int      quota_alloc_irq(owner_t t, l4_uint32_t where);
int      quota_free_mem(owner_t t, l4_addr_t where, unsigned amount);
int      quota_free_task(owner_t t, l4_addr_t where);
int      quota_free_small(owner_t t, l4_addr_t where);
int      quota_free_irq(owner_t t, l4_addr_t where);
void     quota_add_mem(owner_t t, unsigned amount);
quota_t* quota_get(unsigned n);
int      quota_get_log_mcp(owner_t);
unsigned quota_get_offset(owner_t);
void     quota_dump(owner_t t);
void     quota_set_default(quota_t *q);
int      quota_is_default_mem(quota_t *q);
int      quota_is_default_himem(quota_t *q);
int      quota_is_default_task(quota_t *q);
int      quota_is_default_small(quota_t *q);
int      quota_is_default_misc(quota_t *q);

#endif
