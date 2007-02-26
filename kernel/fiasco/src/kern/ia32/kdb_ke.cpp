INTERFACE:

#ifndef jdb_enter_kdebug
#define jdb_enter_kdebug(text) \
asm(\
    "int	$3	\n\t"\
    "jmp	1f	\n\t"\
    ".ascii \""text "\"	\n\t"\
    "1:			\n\t"\
    "nop;nop;nop	\n\t"\
    )
#endif


IMPLEMENTATION:

#include <cstdio>

inline NEEDS [<cstdio>]
bool kdb_ke(const char *msg)
{
  printf("KDB: %s\n",msg);

  asm ("int3            \n\t"
       "jmp 1f		\n\t"
       ".ascii \"KDB\"	\n\t"
       "1:		\n\t");

  return true;
}
