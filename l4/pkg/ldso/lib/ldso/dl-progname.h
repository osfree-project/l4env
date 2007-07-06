const char * _dl_progname="libld-l4.s.so";

#ifdef ARCH_x86
#include <ARCH-x86/elfinterp.c>
#endif

#ifdef ARCH_arm
#include <ARCH-arm/elfinterp.c>
#endif

#ifdef ARCH_amd64
#include <ARCH-amd64/elfinterp.c>
#endif
