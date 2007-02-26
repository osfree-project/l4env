#ifndef _BITS_DLFCN_H
#define _BITS_DLFCN_H

#define RTLD_LAZY	0x00001
#define RTLD_NOW	0x00002

/* If the following bit is set in the MODE argument to `dlopen',
   the symbols of the loaded object and its dependencies are made
   visible as if the object were linked directly into the program.  */
#define RTLD_GLOBAL	0x00100

#endif
