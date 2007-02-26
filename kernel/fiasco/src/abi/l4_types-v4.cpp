INTERFACE:

#include "types.h"

/// L4 Version 4 specific interface.

class Utcb;

EXTENSION class L4_uid
{
public:
  enum { 
    NIL		= 0,
    ANY		= (Mword) -1,
    INVALID	= (Mword) -2,
  };

  /// Create a global UID for the given thread and version.
  L4_uid (Mword thread, Mword version);
  
  /// Create a local UID for the given thread.
  // The two unused parameters are needed to distinguish the
  // three constructors.
  L4_uid (Mword local_threadno, int, int);

  /// Create from the raw 32Bit representation.
  L4_uid( Unsigned32 = NIL );
  
  /**
   * @brief Is this the global ANY ID?
   * @return 0 if it isn't, not 0 else.
   */
  Mword is_any() const;

  /**
   * @brief Is this the local ANY ID?
   * @return 0 if it isn't, not 0 else.
   */
  Mword is_any_local() const;

  /// Extract the raw 32Bit representation.
  Unsigned32 raw() const;

private:
  enum {
    GLOBAL_SHIFT	= 14,
    LOCAL_SHIFT		= 6,	
    GLOBAL_MASK		= (Mword) ~((1 << GLOBAL_SHIFT) - 1),
    VERSION_MASK	= (Mword) ~GLOBAL_MASK,
    LOCAL_MASK		= (Mword) ~((1 << LOCAL_SHIFT) - 1),
  };

  Mword _raw;

public:
  /// must be constant since we build the spaces array from it
  enum { MAX_TASKS     = 1 << 14 };
};

class Msg_tag
{
public:
  Msg_tag (Mword raw);
  Mword untyped_words() const;
  Mword typed_items()	const;
  Mword label()		const;

private:
  enum {
    // layout of the Msg_tag field
    UNTYPED_SHIFT	=  0,
    TYPED_SHIFT		=  6,
    PROPAGATED_BIT	= 12,
    REDIR_BIT		= 13,
    X_PROC_BIT		= 14,
    ERROR_BIT		= 15,
    LABEL_SHIFT		= 16,
    // calculate SIZEs from SHIFTs and BITs
    UNTYPED_SIZE	= TYPED_SHIFT - UNTYPED_SHIFT,
    TYPED_SIZE		= PROPAGATED_BIT - TYPED_SHIFT,
    LABEL_SIZE		= 8 * sizeof (Mword) - LABEL_SHIFT,
    // calculate MASKs from SIZEs and SHIFTs
    UNTYPED_MASK	= (Mword) ((1 << UNTYPED_SIZE) - 1) << UNTYPED_SHIFT,
    TYPED_MASK		= (Mword) ((1 << TYPED_SIZE) - 1) << TYPED_SHIFT,
    LABEL_MASK		= (Mword) ((1 << LABEL_SIZE) - 1) << LABEL_SHIFT,
  };

  Mword _raw;
};

EXTENSION class L4_fpage
{  
public:

  /**
   * @brief Flex page constants.
   */
  enum {
    WHOLE_SPACE = 32, ///< Size of the whole address space.
  };

  /**
   * @brief Create a flexpage with the given parameters.
   *
   * @param read if not zero the read bit is to be set.
   * @param write if not zero the write bit is to be set.
   * @param exec if not zero the exec bit is to be set.
   * @param order the size of the flex page is 2^order.
   * @param base the base address of the flex page.
   *   
   */
  L4_fpage (Mword read, Mword write, Mword exec, Mword order, Mword base);
  
  /**
   * @brief Create a flex page from the binary representation.
   * @param w the binary representation.
   */
  L4_fpage( Mword w = 0 );

  /**
   * @brief Is the read bit set?
   *
   * @return the state of the read bit.
   */
  Mword read() const;

  /**
   * @brief Set the read bit according to g.
   *
   * @param g if not zero the read bit is to be set.
   */
  void read( Mword g );

  /**
   * @brief Is the exec bit set?
   *
   * @return the state of the exec bit.
   */
  Mword exec() const;

  /**
   * @brief Set the exec bit according to g.
   *
   * @param g if not zero the exec bit is to be set.
   */
  void exec( Mword g );

  /**
   * @brief Get the binary representation of the flex page.
   * @return this flex page in binary representation.
   */
  Mword raw() const;

private:
  enum {
    EXEC_BIT	=  0,
    WRITE_BIT	=  1,
    READ_BIT	=  2,
    SIZE_SHIFT	=  4,
    SIZE_SIZE	=  6,
    BASE_SHIFT	= 10,
    BASE_SIZE   = 22,
    SIZE_MASK	= (Mword) ((1 << SIZE_SIZE) - 1) << SIZE_SHIFT,
    BASE_MASK	= (Mword) ((1 << BASE_SIZE) - 1) << BASE_SHIFT,
  };

  Mword _raw;

};

class Utcb 
{
public:
  enum {
    UTCB_PTR_OFFSET = 192,  // the offs. where gs[0] points into the UTCB
  };
  
private:
  Mword buffer_regs[32];
  L4_uid global_id;
  Mword processor;
  Mword userdef_handle;
  Mword pager;
  Mword exception_handler;
  Mword flags;
  Mword error_code;
  Mword xfer_timeouts;
  Mword receiver;
  Mword sender;
  Mword thread_word_1;
  Mword thread_word_0;
  Mword padding[4];
  Mword unused;		// the field where UTCB_PTR_OFFSET points to
  Mword message_regs[63];
};


IMPLEMENTATION[v4]:

//
// L4v4 L4_uid Implementation
//

IMPLEMENT inline L4_uid::L4_uid( Mword threadno, Mword version )
  : _raw ((threadno << GLOBAL_SHIFT) | version) {}

IMPLEMENT inline L4_uid::L4_uid (Mword local_threadno, int, int)
  : _raw (local_threadno & LOCAL_MASK) {}

IMPLEMENT inline L4_uid::L4_uid( Unsigned32 w ) : _raw(w) {}

IMPLEMENT inline unsigned L4_uid::version() const 
{ return (_raw & VERSION_MASK); }

IMPLEMENT inline 
LThread_num L4_uid::lthread() const { return _raw & LOCAL_MASK; }

IMPLEMENT inline
GThread_num L4_uid::gthread() const { return (_raw >> GLOBAL_SHIFT); }

IMPLEMENT inline void L4_uid::version (Mword w)
{ _raw = (_raw & GLOBAL_MASK) | (w & VERSION_MASK); }

IMPLEMENT inline void L4_uid::lthread (Mword w) { _raw = w & LOCAL_MASK; }

IMPLEMENT inline Mword L4_uid::is_nil() const { return _raw == NIL; }
IMPLEMENT inline Mword L4_uid::is_invalid() const { return _raw == INVALID; }
IMPLEMENT inline Mword L4_uid::is_any() const { return _raw == ANY; }

IMPLEMENT inline Mword L4_uid::is_any_local() const 
{ return _raw == LOCAL_MASK; }


// get the (global) L4_uid for a given IRQ
IMPLEMENT inline L4_uid L4_uid::irq (Mword int_no)
{ return L4_uid ((int_no << GLOBAL_SHIFT) | 1); }

IMPLEMENT inline Mword L4_uid::is_irq() const 
{
  // XXX not exactly specified.  Maybe we have to check if 
  // gthread < kip.thread_info.system_base, too.
  return version() == 1;
}

IMPLEMENT inline Mword L4_uid::irq() const
{ return _raw >> GLOBAL_SHIFT; }

IMPLEMENT inline bool L4_uid::operator == ( L4_uid o ) const 
{ return o._raw == _raw; }

IMPLEMENT inline Unsigned32 L4_uid::raw() const { return _raw; }

/// Check if the given ID is a local ID
PUBLIC inline Mword L4_uid::is_local()
{ return !is_nil() && !(_raw & ~LOCAL_MASK); }

/** UTCB pointer
 * @pre is_local() true
 * @return Pointer to the corresponding UTCB
 */
PUBLIC inline Utcb *L4_uid::utcb() const
{ return (Utcb *) (lthread() - Utcb::UTCB_PTR_OFFSET); }

//
// L4v4 Msg_tag Implementation
//

IMPLEMENT inline Msg_tag::Msg_tag (Mword raw) : _raw (raw) {};

IMPLEMENT inline Mword Msg_tag::untyped_words() const
{ return (_raw & UNTYPED_MASK) >> UNTYPED_SHIFT; }

IMPLEMENT inline Mword Msg_tag::typed_items() const
{ return (_raw & TYPED_MASK) >> TYPED_SHIFT; }

IMPLEMENT inline Mword Msg_tag::label() const
{ return (_raw & LABEL_MASK) >> LABEL_SHIFT; }

//
// L4v4 L4_fpage Implementation
//

IMPLEMENT inline L4_fpage::L4_fpage (Mword read, Mword write, Mword exec,
				     Mword size, Mword base)
  :_raw ((read ? (1 << READ_BIT) : 0)
	 | (write ? (1 << WRITE_BIT) : 0)
	 | (exec ? (1 << EXEC_BIT) : 0)
	 | ((size << SIZE_SHIFT) & SIZE_MASK)
	 | (base & BASE_MASK)
	 )
{}

IMPLEMENT inline L4_fpage::L4_fpage (Mword raw) 
  : _raw (raw)
{}

IMPLEMENT inline Mword L4_fpage::read() const 
{ return _raw & (1 << READ_BIT); }

IMPLEMENT inline void L4_fpage::read (Mword b) 
{ 
  if (b) _raw |=  (1 << READ_BIT); 
  else   _raw &= ~(1 << READ_BIT); 
}

IMPLEMENT inline Mword L4_fpage::write() const 
{ return _raw & (1 << WRITE_BIT); }

IMPLEMENT inline void L4_fpage::write (Mword b)
{ 
  if (b) _raw |=  (1 << WRITE_BIT); 
  else   _raw &= ~(1 << WRITE_BIT); 
}

IMPLEMENT inline Mword L4_fpage::exec() const 
{ return _raw & (1 << EXEC_BIT); }

IMPLEMENT inline void L4_fpage::exec (Mword b)
{ 
  if (b) _raw |=  (1 << EXEC_BIT); 
  else   _raw &= ~(1 << EXEC_BIT);
}

IMPLEMENT inline Mword L4_fpage::size() const 
{ return (_raw & SIZE_MASK) >> SIZE_SHIFT; }

IMPLEMENT inline Mword L4_fpage::page() const 
{ return _raw & BASE_MASK; }

IMPLEMENT inline void L4_fpage::page (Mword w)
{ _raw = _raw & ~BASE_MASK | (w & BASE_MASK); }

IMPLEMENT inline Mword L4_fpage::is_whole_space() const 
{ return (size() == 1 && page() == 0); }

IMPLEMENT inline Mword L4_fpage::is_valid() const
{ return _raw; }

IMPLEMENT inline Mword L4_fpage::raw() const
{ return _raw; }

//
// XXX Methods to be eliminated in v4
//
//PUBLIC L4_uid::L4_uid(Mword, Mword, Mword, Mword) {}
PUBLIC inline Task_num L4_uid::task() const {return 0;}
PUBLIC inline L4_uid L4_uid::task_id() const {return 0;}
PUBLIC inline Mword L4_uid::chief() const {return 0;}
PUBLIC inline void L4_uid::task (Task_num) {}
PUBLIC inline void L4_uid::chief (Task_num) {}
PUBLIC static Mword L4_uid::threads_per_task() {return 0;}
PUBLIC L4_fpage::L4_fpage (Mword, Mword, Mword, Mword) {}
//
// XXX Methods to be moved somewhere else in v4
//
PUBLIC inline bool L4_uid::abs_recv_timeout() const {return false;}
PUBLIC inline bool L4_uid::abs_send_timeout() const {return false;}
PUBLIC inline bool L4_uid::abs_recv_clock() const   {return false;}
PUBLIC inline bool L4_uid::abs_send_clock() const   {return false;}

//
// L4v4 UTCB implementation
//
PUBLIC Utcb::Utcb() {}

