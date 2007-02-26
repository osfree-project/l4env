#ifndef GLOBALS_H
#define GLOBALS_H

#include <l4/sys/types.h>

/* Special options for compatibility reasons */

enum {
  debug_errors        = 1,
  debug_warnings      = 0,
  debug_ipc           = 0,
  debug_memory_maps   = 0,

};

/* globals defined here (when included from globals.c) */
#define PROG_NAME "SIGMA0"

enum {
  sigma0_taskno = 2,
  root_taskno = 4
};

#endif
