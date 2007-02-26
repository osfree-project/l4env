#ifndef KDB_KE_H
#define KDB_KE_H

#define kdb_ke(msg)			\
  asm ("int3           		\n\t"	\
       "jmp 1f			\n\t"	\
       ".ascii \"" msg "\"	\n\t"	\
       "1:			\n\t");

#define kdb_ke_sequence(msg)		\
  asm ("int3			\n\t"	\
       "jmp 1f			\n\t"	\
       ".ascii \"*##\"		\n\t"	\
       "1:			\n\t"	\
       : : "a"(msg));

#endif
