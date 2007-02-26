#ifndef CFG_H
#define CFG_H

#include <l4/sys/compiler.h>
#include "quota.h"
#include "bootquota.h"
#include "rmgr.h"

void cfg_setup_input(const char *cfg_buffer, const char *cfg_buffer_end);
int  __cfg_parse(void);

extern int __cfg_task;
extern quota_t q;
extern bootquota_t b;

L4_INLINE void cfg_init(void);
L4_INLINE int  cfg_parse(void);

L4_INLINE void
cfg_init(void)
{
  /* first task the config file configures */
  __cfg_task = TASKNO_ROOT;
  quota_set_default(&q);
  bootquota_set_default(&b);
}

extern unsigned mem_used;
void cfg_done(void);
void show_mem_used(void);

L4_INLINE int
cfg_parse(void)
{
  int r;
  r = __cfg_parse();
  cfg_done();
  return r;
}

void find_module(int *cfg_mod, char* cfg_name);
void check_module(unsigned cfg_mod, char *cfg_name);

#endif
