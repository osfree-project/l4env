#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "svnversion.h"

const char* dice_build   = __DATE__ " " __TIME__;
#ifdef __USER__
const char* dice_user    = __USER__;
#else
const char* dice_user    = 0;
#endif
#ifdef SVNVERSION
const char* dice_svnrev  = SVNVERSION;
#else
const char* dice_svnrev  = 0;
#endif
