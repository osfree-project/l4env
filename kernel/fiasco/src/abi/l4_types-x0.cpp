INTERFACE:

#include "types.h"

// L4 Version x0 specific interface.
EXTENSION class L4_uid
{
public:

  /// The standard IDs.
  enum { 
    INVALID = 0xffffffff,
    NIL     = 0x00000000,
  };

  /// Create a UID for the given task and thread.
  L4_uid( Task_num task, LThread_num lthread );
  
  /// Extract the task number.
  Task_num task() const;

  /// Extract the chief ID.
  Task_num chief() const;

  /// Set the task number.
  void task( Task_num );

  /// Set the chief ID.
  void chief( Task_num );

  /// Get the task ID, thle local thread-number set to 0.
  L4_uid task_id() const;

  /**
   * @brief Check if receive timeout is absolute.
   */
  bool abs_recv_timeout() const;

  /**
   * @brief Check if send timeout is absolute.
   */
  bool abs_send_timeout() const;

  /**
   * @brief Check if receive timeout clock bit set.
   */
  bool abs_recv_clock() const;

  /**
   * @brief Check if send timeout clock bit set.
   */
  bool abs_send_clock() const;

  /// Get the maximum number of threads per task.
  static unsigned const threads_per_task();

  /// Create from the raw 32Bit representation.
  L4_uid( Unsigned32 = NIL );
  
  /// Create L4 X0 UID.
  L4_uid( Task_num task, LThread_num lthread, Task_num chief, 
	  unsigned version = 0 );

  /// Extract the raw 32Bit representation.
  Unsigned32 raw() const;

  /** Get number of threads in the system.
   * This method only works for v2 and x0 ABI.
   * To get the max_threads value ABI-independent, use 
   * Kmem::info()->max_threads() instead.
   */
  static Mword const max_threads();

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

    ABS_RECV_MASK      = 1,
    ABS_SEND_MASK      = 2,
    ABS_RECV_CLOCK     = 4,
    ABS_SEND_CLOCK     = 8,
  };

  Unsigned32 _raw;

public:
  /// must be constant since we build the spaces array from it
  enum { MAX_TASKS     = 1 << TASK_SIZE };
};


EXTENSION class L4_fpage
{
public:
  /**
   * @brief Create a flexpage with the given parameters.
   *
   * @param grant if not zero the grant bit is to be set.
   * @param write if not zero the write bit is to be set.
   * @param order the size of the flex page is 2^order.
   * @param page the base address of the flex page.
   *   
   */
  L4_fpage( Mword grant, Mword write, Mword order, Mword page );

  /**
   * @brief Is the grant bit set?
   *
   * @return the state of the grant bit.
   */
  Mword grant() const;

  /**
   * @brief Set the grant bit according to g.
   *
   * @param g if not zero the grant bit is to be set.
   */
  void grant( Mword g );

};



IMPLEMENTATION[x0]:

IMPLEMENT inline
bool L4_uid::abs_recv_timeout() const
{
  return chief() & ABS_RECV_MASK;
}
 
IMPLEMENT inline
bool L4_uid::abs_send_timeout() const
{
  return chief() & ABS_SEND_MASK;
}
 
IMPLEMENT inline
bool L4_uid::abs_recv_clock() const
{
  return chief() & ABS_RECV_CLOCK;
}
 
IMPLEMENT inline
bool L4_uid::abs_send_clock() const
{
  return chief() & ABS_SEND_CLOCK;
}

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

IMPLEMENT inline
Mword const L4_uid::max_threads()
{
  return 1 << (TASK_SIZE + LTHREAD_SIZE);
}

