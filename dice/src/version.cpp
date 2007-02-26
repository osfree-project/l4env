#if HAVE_CONFIG_H
#include <config.h>
#endif

const char* dice_build   = __DATE__ " " __TIME__;
const char* dice_version = VERSION;
#ifdef __USER__
const char* dice_user    = __USER__;
#else
const char* dice_user    = 0;
#endif
const char* dice_configure_gcc     = __CONFIGURECXX__;
const char* dice_compile_gcc       = __WHICHCXX__;

