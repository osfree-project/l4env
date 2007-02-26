/*
 * Fiasco
 * Floating point unit code
 */

INTERFACE:

#include "initcalls.h"

class slab_cache_anon;
class Context;
class Fpu_state;


class Fpu 
{
public:

  static Context *owner();
  static void set_owner(Context *owner);
  static bool is_owner(Context *owner);

  // all the following methods are arch dependent
  static void init() FIASCO_INIT;
  static unsigned const state_size();
  static unsigned const state_align();
  static void init_state( Fpu_state *);
  static void restore_state( Fpu_state * );
  static void save_state( Fpu_state * );
  static void disable();
  static void enable();

private:

  static Context *_owner;
};

IMPLEMENTATION:

#include "fpu_state.h"

Context *Fpu::_owner;

IMPLEMENT inline
Context * Fpu::owner()
{
  return _owner;
}

IMPLEMENT inline
void Fpu::set_owner(Context *owner)
{
  _owner = owner;
}

IMPLEMENT inline
bool Fpu::is_owner(Context *owner)
{
  return _owner == owner;
}

//---------------------------------------------------------------------------
IMPLEMENTATION [!fpu]:

IMPLEMENT inline
void Fpu::init_state (Fpu_state *)
{}

IMPLEMENT inline
unsigned const Fpu::state_size()
{
  return 0;
}

IMPLEMENT inline
unsigned const Fpu::state_align()
{
  return 0;
}

IMPLEMENT
void Fpu::init() 
{}
   
IMPLEMENT inline
void Fpu::save_state( Fpu_state * ) 
{}  


IMPLEMENT inline
void Fpu::restore_state( Fpu_state * ) 
{}

IMPLEMENT inline
void Fpu::disable()
{}

IMPLEMENT inline
void Fpu::enable()
{}

