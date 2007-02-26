IMPLEMENTATION[ux]:

#include "initcalls.h"

IMPLEMENT FIASCO_INIT
void
Timer::init()
{ 
  init_done = true;
}

IMPLEMENT inline 
void Timer::acknowledge()
{
}

IMPLEMENT inline 
void Timer::enable()
{
}

IMPLEMENT inline 
void Timer::disable()
{
}
