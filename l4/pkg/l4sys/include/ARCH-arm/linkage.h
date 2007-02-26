#ifndef LINKAGE_H
#define LINKAGE_H

#ifdef __ASSEMBLER__

#define ENTRY(name) \
  .globl name; \
  .p2align(2); \
  name:

#endif

#endif /* LINKAGE_H */
