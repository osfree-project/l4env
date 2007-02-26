IMPLEMENTATION[debug]:

#include <cstdio>
#include "simpleio.h"

PUBLIC void L4_uid::print(int task_format=0)
{
  if(is_invalid()) 
    putstr("---.--");
  else if(is_irq())
    printf("irq-%02x",irq());
  else 
    printf("%*x.%02x",task_format, task(), lthread());
}

