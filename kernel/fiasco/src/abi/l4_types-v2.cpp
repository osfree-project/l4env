INTERFACE:

#include "types.h"

// L4 Version 2 specific interface.
EXTENSION class L4_uid
{
public:

  /// The standard constants for:
  enum { 
    INVALID = 0x0ffffffff, ///< The INVALID ID
    NIL     = 0x000000000, ///< The NIL ID
  };

  /// Create a L4 UID from the binary 64Bit representation.
  L4_uid( Unsigned64 = NIL );
  
  /// Create a L4 V2 UID.
  L4_uid( Task_num task, LThread_num lthread, unsigned site,
	  Task_num chief, unsigned nest = 0, unsigned version = 0 );

  /// Extract the V2 specific site ID.
  unsigned site() const;

  /// Extract the Clans&Chiefs nesting level.
  unsigned nest() const;

  /// Set the site ID (L4V2).
  void site( unsigned );

  /// Set the C&C nesting level (L4V2).
  void nest( unsigned );

  /// Get the UID in binary 64Bit representation.
  Unsigned64 raw() const;


private:
  enum {
    IRQ_MASK           = 0x00000000000000ffLL,
    LOW_MASK           = 0x00000000ffffffffLL,
    VERSION_LOW_MASK   = 0x00000000000003ffLL,
    VERSION_LOW_SHIFT  = 0,
    VERSION_LOW_SIZE   = 10,
    LTHREAD_MASK       = 0x000000000001fc00LL,
    LTHREAD_SHIFT      = VERSION_LOW_SHIFT + VERSION_LOW_SIZE,
    LTHREAD_SIZE       = 7,
    TASK_MASK          = 0x000000000ffe0000LL,
    TASK_SHIFT         = LTHREAD_SHIFT + LTHREAD_SIZE,
    TASK_SIZE          = 11,
    VERSION_HIGH_MASK  = 0x00000000f0000000LL,
    VERSION_HIGH_SHIFT = TASK_SHIFT + TASK_SIZE,
    VERSION_HIGH_SIZE  = 4,
    SITE_MASK          = 0x0001ffff00000000LL,
    SITE_SHIFT         = VERSION_HIGH_SHIFT + VERSION_HIGH_SIZE,
    SITE_SIZE          = 17,
    CHIEF_MASK         = 0x0ffe000000000000LL,
    CHIEF_SHIFT        = SITE_SHIFT + SITE_SIZE,
    CHIEF_SIZE         = 11,
    NEST_MASK          = 0xf000000000000000LL,
    NEST_SHIFT         = CHIEF_SHIFT + CHIEF_SIZE,
    NEST_SIZE          = 4,
  };

  Unsigned64 _raw;

};


IMPLEMENTATION[v2]:

IMPLEMENT inline
unsigned const L4_uid::threads_per_task()
{
  return 1<<LTHREAD_SIZE;
}

IMPLEMENT inline
bool L4_uid::operator == ( L4_uid o ) const
{
  return o._raw == _raw;
}

#if 0
IMPLEMENT inline
bool L4_uid::operator != ( L4_uid o ) const
{
  return o._raw != _raw;;
}
#endif


IMPLEMENT inline
L4_uid::L4_uid( Unsigned64 w )
  : _raw(w)
{}


IMPLEMENT inline 
L4_uid::L4_uid( Task_num task, LThread_num lthread )
  : _raw(   (((Unsigned64)task    << TASK_SHIFT   ) & TASK_MASK   )
	  | (((Unsigned64)lthread << LTHREAD_SHIFT) & LTHREAD_MASK))
{}


IMPLEMENT inline 
L4_uid::L4_uid( Task_num task, LThread_num lthread, unsigned site,
		Task_num chief, unsigned nest, unsigned version )
  : _raw (  (((Unsigned64)task    << TASK_SHIFT)         & TASK_MASK        )
	  | (((Unsigned64)lthread << LTHREAD_SHIFT)      & LTHREAD_MASK     )
	  | (((Unsigned64)site    << SITE_SHIFT)         & SITE_MASK        )
	  | (((Unsigned64)chief   << CHIEF_SHIFT)        & CHIEF_MASK       )
	  | (((Unsigned64)nest    << NEST_SHIFT)         & NEST_MASK        )
	  | (((Unsigned64)version << VERSION_LOW_SHIFT)  & VERSION_LOW_MASK )
	  | (((Unsigned64)version << VERSION_HIGH_SHIFT) & VERSION_HIGH_MASK) )
{}

IMPLEMENT inline
unsigned L4_uid::version() const
{
  return ((_raw & VERSION_HIGH_MASK) >> VERSION_HIGH_SHIFT)
       | ((_raw & VERSION_LOW_MASK ) >> VERSION_LOW_SHIFT);
}

IMPLEMENT inline
LThread_num L4_uid::lthread() const
{
  // cast to unsigned is hint for gcc
  return ((unsigned)_raw & LTHREAD_MASK) >> LTHREAD_SHIFT;
}

IMPLEMENT inline
Task_num L4_uid::task() const
{
  // cast to unsigned is hint for gcc
  return ((unsigned)_raw & TASK_MASK) >> TASK_SHIFT;
}

IMPLEMENT inline
GThread_num L4_uid::gthread() const
{
  // cast to unsigned is hint for gcc
  return (((unsigned)_raw & LTHREAD_MASK) >> LTHREAD_SHIFT) | 
	 (((unsigned)_raw & TASK_MASK   ) >> (TASK_SHIFT - LTHREAD_SIZE));
}

IMPLEMENT inline
unsigned L4_uid::site() const
{
  return (_raw & SITE_MASK) >> SITE_SHIFT;
}

IMPLEMENT inline
unsigned L4_uid::chief() const
{
  return (_raw & CHIEF_MASK) >> CHIEF_SHIFT;
}

IMPLEMENT inline
unsigned L4_uid::nest() const
{
  return (_raw & NEST_MASK) >> NEST_SHIFT;
}


IMPLEMENT inline
void L4_uid::version( unsigned w )
{
  _raw = (_raw & ~(VERSION_LOW_MASK | VERSION_HIGH_MASK)) 
       | (((Unsigned64)w << VERSION_LOW_SHIFT)  & VERSION_LOW_MASK)
       | (((Unsigned64)w << VERSION_HIGH_SHIFT) & VERSION_HIGH_MASK);
}

IMPLEMENT inline
void L4_uid::lthread( LThread_num w )
{
  _raw = (_raw & ~LTHREAD_MASK) 
       | (((Unsigned64)w << LTHREAD_SHIFT) & LTHREAD_MASK);
}

IMPLEMENT inline
void L4_uid::task( Task_num w )
{
  _raw = (_raw & ~TASK_MASK) 
       | (((Unsigned64)w << TASK_SHIFT) & TASK_MASK);
}


IMPLEMENT inline
void L4_uid::site( unsigned w )
{
  _raw = (_raw & ~SITE_MASK) | (((Unsigned64)w << SITE_SHIFT) & SITE_MASK);
}


IMPLEMENT inline
void L4_uid::chief( Task_num w )
{
  _raw = (_raw & ~CHIEF_MASK) | (((Unsigned64)w << CHIEF_SHIFT) & CHIEF_MASK);
}


IMPLEMENT inline
void L4_uid::nest( unsigned w )
{
  _raw = (_raw & ~NEST_MASK) | (((Unsigned64)w << NEST_SHIFT) & NEST_MASK);
}

IMPLEMENT inline
Mword L4_uid::is_nil() const
{
  return (_raw & LOW_MASK) == 0;
}

IMPLEMENT inline
Mword L4_uid::is_invalid() const
{
  return (_raw & LOW_MASK) == INVALID;
}

IMPLEMENT inline
L4_uid L4_uid::task_id() const
{
  return L4_uid( _raw & ~LTHREAD_MASK );
}

IMPLEMENT inline
L4_uid L4_uid::irq( unsigned irq )
{
  return L4_uid( (irq + 1) & IRQ_MASK );
}

IMPLEMENT inline
Mword L4_uid::is_irq() const
{
  return (_raw & ~IRQ_MASK) == 0 && _raw;
}

IMPLEMENT inline
Mword L4_uid::irq() const
{
  return _raw-1;
}

IMPLEMENT inline
Unsigned64 L4_uid::raw() const
{
  return _raw;
}

  
