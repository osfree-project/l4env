INTERFACE:

#include "types.h"

EXTENSION class L4_uid
{
public:

  /// The standard IDs.
  enum { 
    INVALID = 0xffffffff,
    NIL     = 0x00000000,
  };

  /// Create from the raw 32Bit representation.
  L4_uid( Unsigned32 = NIL );
  
  /// Create L4 X0 UID.
  L4_uid( Task_num task, LThread_num lthread, Task_num chief, 
	  unsigned version = 0 );

  /// Extract the raw 32Bit representation.
  Unsigned32 raw() const;

private:
  enum {
    IRQ_MASK           = 0x000000ff,
    VERSION_MASK       = 0x000003ff,
    VERSION_SHIFT      = 0,
    VERSION_SIZE       = 10,
    LTHREAD_MASK       = 0x0000fc00,
    LTHREAD_SHIFT      = VERSION_SHIFT + VERSION_SIZE,
    LTHREAD_SIZE       = 6,
    TASK_MASK          = 0x00ff0000,
    TASK_SHIFT         = LTHREAD_SHIFT + LTHREAD_SIZE,
    TASK_SIZE          = 8,
    CHIEF_MASK         = 0xff000000,
    CHIEF_SHIFT        = TASK_SHIFT + TASK_SIZE,
    CHIEF_SIZE         = 8,
  };

  Unsigned32 _raw;

};


IMPLEMENTATION[x0]:

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
L4_uid::L4_uid( Unsigned32 w )
  : _raw(w)
{}

IMPLEMENT inline 
L4_uid::L4_uid( Task_num task, LThread_num lthread )
  : _raw( (((Unsigned32)task << TASK_SHIFT) & TASK_MASK) 
	  | (((Unsigned32)lthread << LTHREAD_SHIFT) & LTHREAD_MASK))
{}

IMPLEMENT inline 
L4_uid::L4_uid( Task_num task, LThread_num lthread, 
		Task_num chief, unsigned version )
  : _raw( (((Unsigned32)task << TASK_SHIFT) & TASK_MASK) 
	  | (((Unsigned32)lthread << LTHREAD_SHIFT) & LTHREAD_MASK)
	  | (((Unsigned32)chief << CHIEF_SHIFT) & CHIEF_MASK)
	  | (((Unsigned32)version << VERSION_SHIFT) & VERSION_MASK))
{}

IMPLEMENT inline
unsigned L4_uid::version() const
{
  return ((_raw & VERSION_MASK) >> VERSION_SHIFT); 
}

IMPLEMENT inline
LThread_num L4_uid::lthread() const
{
  return (_raw & LTHREAD_MASK) >> LTHREAD_SHIFT;
}

IMPLEMENT inline
Task_num L4_uid::task() const
{
  return (_raw & TASK_MASK) >> TASK_SHIFT;
}

IMPLEMENT inline
GThread_num L4_uid::gthread() const
{
  return ((_raw & LTHREAD_MASK) >> LTHREAD_SHIFT) | 
    ((_raw & TASK_MASK) >> (TASK_SHIFT - LTHREAD_SIZE));
}

IMPLEMENT inline
Task_num L4_uid::chief() const
{
  return (_raw & CHIEF_MASK) >> CHIEF_SHIFT;
}

IMPLEMENT inline
void L4_uid::version( unsigned w )
{
  _raw = (_raw & ~VERSION_MASK) | (((Unsigned32)w << VERSION_SHIFT) & VERSION_MASK);
}

IMPLEMENT inline
void L4_uid::lthread( LThread_num w )
{
  _raw = (_raw & ~LTHREAD_MASK) | (((Unsigned32)w << LTHREAD_SHIFT) & LTHREAD_MASK);
}

IMPLEMENT inline
void L4_uid::task( Task_num w )
{
  _raw = (_raw & ~TASK_MASK) | (((Unsigned32)w << TASK_SHIFT) & TASK_MASK);
}


IMPLEMENT inline
void L4_uid::chief( Task_num w )
{
  _raw = (_raw & ~CHIEF_MASK) | (((Unsigned32)w << CHIEF_SHIFT) & CHIEF_MASK);
}

IMPLEMENT inline
Mword L4_uid::is_nil() const
{
  return _raw == 0;
}

IMPLEMENT inline
Mword L4_uid::is_invalid() const
{
  return _raw == INVALID;
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
Unsigned32 L4_uid::raw() const
{
  return _raw;
}
