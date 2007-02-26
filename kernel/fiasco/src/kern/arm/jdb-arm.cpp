INTERFACE:

EXTENSION class Jdb_entry_frame
{
public:
  Unsigned32 r[15];
  Unsigned32 spsr;
  Unsigned32 cpsr;
  Unsigned32 ksp;
  Unsigned32 pc;
};

IMPLEMENTATION[arm]:

#include "globals.h"

IMPLEMENT 
Context *Jdb::current_context()
{
  return context_of((void*)entry_frame->ksp);
}
