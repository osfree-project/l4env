/**
 * \file	roottask/server/src/bootquota.h
 * \brief	bootquota handling
 *
 * \date	05/10/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author      Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 **/

#include <stdio.h>
#include "string.h"
#include "bootquota.h"
#include "rmgr.h"
#include "types.h"
#include "misc.h"

/* We store only the pointer to the name. */
typedef struct
{
  const char  *name;
  bootquota_t bootquota;
} cfg_bootquota_t;

static bootquota_t     bootquota[RMGR_TASK_MAX]; /* boot quotas (cfg-parse.y) */
static cfg_bootquota_t cfg_bootquota[RMGR_CFG_MAX];
static int             max_cfg_bootquota;


void
cfg_bootquota_set(const char *cfg_name, const bootquota_t * const b)
{
  int j;

  for (j = 0; j < max_cfg_bootquota; j++)
    if (!strcmp(cfg_bootquota[j].name, cfg_name))
      {
	/* merge quotas */
	bootquota_t *c = &cfg_bootquota[j].bootquota;
	if (b->mcp != 255)
	  c->mcp = b->mcp;
	if (b->prio != 0x10)
	  c->prio = b->prio;
	if (b->small_space != 0xff)
	  c->small_space = b->small_space;
	if (b->mods != 0)
	  c->mods = b->mods;
	return;
      }

  /* add new quota */
  if (max_cfg_bootquota < sizeof(cfg_bootquota)/sizeof(cfg_bootquota[0]))
    {
      cfg_bootquota[max_cfg_bootquota].name      = cfg_name;
      cfg_bootquota[max_cfg_bootquota].bootquota = *b;
      max_cfg_bootquota++;
    }
}

void
cfg_bootquota_copy(unsigned i, const char *name)
{
  int j;

  for (j = 0; j < max_cfg_bootquota; j++)
    if (is_program_in_cmdline(name, cfg_bootquota[j].name))
      {
	bootquota[i] = cfg_bootquota[j].bootquota;
	return;
      }
}

void
bootquota_init(void)
{
  unsigned i;

  for (i = 0; i < sizeof(bootquota)/sizeof(bootquota[0]); i++)
    {
      bootquota[i].mcp    = 255;	/* L4 default */
      bootquota[i].prio   = 0x10;	/* L4 default */
      bootquota[i].small_space = 0xff; /* for L4, 0xff means "no change" */
      bootquota[i].mods   = 0;	/* number of boot mods assn'd to this task */
    }
}

void
bootquota_set(unsigned i, bootquota_t *b)
{
  bootquota[i] = *b;
}

bootquota_t*
bootquota_get(unsigned n)
{
  return &bootquota[n];
}

void
bootquota_set_default(bootquota_t *b)
{
  b->mcp         = 255;
  b->prio        = 0x10;
  b->small_space = 0xff;
  b->mods        = 0;
}

int
bootquota_is_default(bootquota_t *b)
{
  return b->mcp         == 255  &&
         b->prio        == 0x10 &&
	 b->small_space == 0xff &&
	 b->mods        == 0;
}
