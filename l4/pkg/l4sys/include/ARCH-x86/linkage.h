#ifndef _LINKAGE_H
#define _LINKAGE_H

#ifdef __ASSEMBLY__

#ifndef ENTRY
#define ENTRY(name) \
  .globl name; \
  .p2align(2); \
  name:

#endif
#endif /* ! ENTRY */

#endif /* _LINKAGE_H */
