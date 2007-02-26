/*
 * arch. independent L4 Types
 */

INTERFACE:

#include "types.h"

/// Type for task numbers.
typedef Mword Task_num;

/// Type for thread numbers (task local).
typedef Mword LThread_num;

/// Type for global thread numbers.
typedef Mword GThread_num;

/**
 * @brief L4 unique ID.
 *
 * This class encapsulates UIDs in L4. Theese IDs are
 * used for threads, tasks, and IRQs.
 *
 * This is the general interface to access L4 Version 2
 * and also L4 Version X.0 UIDs, like specified in the
 * resp. L4 reference manuals.
 */
class L4_uid
{
public:
  
  /**
   * @brief Extract the version part of the UID.
   * @return The version (V2: low and high) of the UID.
   */
  unsigned version() const;

  /// Extract the task local thread-number.
  LThread_num lthread() const;

  /// Extract the system global thread-number.
  GThread_num gthread() const;

  /// Set the version part.
  void version( unsigned );

  /// Set the local thread-number.
  void lthread( LThread_num );

  /**
   * @brief Is this a NIL ID?
   * @return 0 if this is not the NIL ID, not 0 else.
   */
  Mword is_nil() const;

  /**
   * @brief Is this the INVALID ID?
   * @return 0 if this is not the INVALID ID, not 0 else.
   */
  Mword is_invalid() const;

  /// Get the L4 UID for the given IRQ.
  static L4_uid irq( unsigned irq );
  
  /// Is this a IRQ ID?
  Mword is_irq() const;

  /**
   * @brief Get the IRQ number.
   * @pre is_irq() must return true, or the result is unpredictable.
   * @return the IRQ number for the L4 IRQ ID.
   */
  Mword irq() const;

  /// Test for equality.
  bool operator == ( L4_uid o ) const;

  /// Test for inequality.
  bool operator != ( L4_uid o ) const
  { return ! operator == (o); }
};

/**
 * @brief A L4 flex page.
 *
 * A flex page represents a size aligned 
 * region of an address space.
 */
class L4_fpage
{
public:

  // Constructors are defined in the ABI specific files

  /**
   * @brief Is the write bit set?
   *
   * @return the state of the write bit.
   */
  Mword write() const;

  /**
   * @brief Set the write bit according to w.
   *
   * @param w if not zero the write bit is to be set.
   */
  void write( Mword w );
  
  /**
   * @brief Get the size part of the flex page.
   * @return the order of the flex page (size part).
   */
  Mword size() const;

  /**
   * @brief Set the size part of the flex page.
   * @param order the size is 2^order.
   */
  void size( Mword order );

  /**
   * @brief Get the base address of the flex page.
   * @return the base address.
   */
  Mword page() const;

  /**
   * @brief Set the base address of the flex page.
   * @param base the flex page's base address.
   */
  void page( Mword base );

  /**
   * @brief Is the flex page the whole address space?
   * @return not zero, if the flex page covers the 
   *   whole address space.
   */
  Mword is_whole_space() const;

  /**
   * @brief Is the flex page valid?
   * @return not zero if the flex page 
   *    contains a value other than 0.
   */
  Mword is_valid() const;
};

/**
 * @brief L4 send descriptor.
 */
class L4_snd_desc
{
public:

  /**
   * @brief Create a send descriptor from its binary representation.
   */
  L4_snd_desc( Mword w = (Mword)-1 );
  
  /**
   * @brief Deceite bit set?
   */
  Mword deceite() const;

  /**
   * @brief Map bit set?
   */
  Mword map() const;

  /**
   * @brief Get the message base address.
   */
  void *msg() const;

  /**
   * @brief Is the message a long IPC?
   */
  Mword is_long_ipc() const;

  /**
   * @brief Send a register only message?
   * @return true, if a register only message shall be sended.
   */
  Mword is_register_ipc() const;

  /**
   * @brief has the IPC a send part (descriptor != -1).
   */
  Mword has_send() const;

private:
  Mword _d;
};

/**
 * @brief L4 receive descriptor.
 */
class L4_rcv_desc
{
public:

  /**
   * @brief Create a receive descriptor for receiving the given flex page.
   *
   * The descriptor is for receiving the given flex page by an register only
   * message.
   * @param fp the receive flex page.
   * @return An L4 receive descriptor for receiving the given flex page 
   *         as register only message.
   *  
   */
  static L4_rcv_desc short_fpage( L4_fpage fp );

  /**
   * @brief Create a L4 receive descriptor from it's binary representation.
   */
  L4_rcv_desc( Mword w = (Mword)-1 );

  /**
   * @brief Open wait?
   * @return the open wait bit.
   */
  Mword open_wait() const;

  /**
   * @brief Wait for a register map message?
   * @return whether a register map receive is expected.
   */
  Mword rmap() const;

  /**
   * @brief Get a pointer to the receive buffer.
   * @pre The is_register_ipc() method must return false.
   * @return the address of the receive buffer.
   */
  void *msg() const;

  /**
   * @brief Get the receive flex page.
   * @pre rmap() must return true.
   * @return the receive flex page.
   */
  L4_fpage fpage() const;

  /**
   * @brief Receive a register only message?
   * @return true, if a register only message shall be received.
   */
  Mword is_register_ipc() const;

  /**
   * @brief Is there a receive part?
   * @return whether there is a receive expected or not.
   */
  Mword has_receive() const;

private:
  Mword _d;
  
};

/**
 * @brief L4 timeouts data type.
 */
class L4_timeout
{
private:
  enum {
    RCV_EXP_MASK     = 0x0f,
    RCV_EXP_SHIFT    = 0,
    
    SND_EXP_MASK     = 0x0f0,
    SND_EXP_SHIFT    = 4,

    RCV_PFAULT_MASK  = 0x0f00,
    RCV_PFAULT_SHIFT = 8,

    SND_PFAULT_MASK  = 0x0f000,
    SND_PFAULT_SHIFT = 12,

    SND_MAN_MASK     = 0x0ff0000,
    SND_MAN_SHIFT    = 16,
    
    RCV_MAN_MASK     = 0x0ff000000,
    RCV_MAN_SHIFT    = 24,
  };

public:
  /// Typical timout constants.
  enum {
    NEVER = 0, ///< Never time out.
  };

  /**
   * @brief Create a timeout from it's binary representation.
   * @param t the binary timeout value.
   */
  explicit L4_timeout( Mword t = 0 );

  /**
   * @brief Create the specified timeout.
   * @param snd_man mantissa of the send timeout.
   * @param snd_exp exponent of the send timeout 
   *        (snd_exp=0: infinite timeout,
   *        snd_exp>0: t=4^(15-snd_exp)*snd_man, 
   *        snd_man=0 & snd_exp!=0: t=0).
   * @param rcv_man mantissa of the receive timeout.
   * @param rcv_exp exponent of the receive timeout (see snd_exp).
   * @param snd_pflt send page fault timeout (snd_pflt=0: infinite timeout,
   *        0<snd_pflt<15: t=4^(15-snd_pflt), snd_pflt=15: t=0).
   * @param rcv_pflt receive page fault timeout (see snd_pflt).
   */
  L4_timeout( Mword snd_man, Mword snd_exp, 
	      Mword rcv_man, Mword rcv_exp, 
	      Mword snd_pflt, Mword rcv_pflt );

  /**
   * @brief Get the binary representation of the timeout.
   * @return The timeout as binary representation.
   */
  Unsigned64 raw();

  /**
   * @brief Get the receive exponent.
   * @return The exponent of the receive timeout.
   * @see rcv_man()
   */
  Mword rcv_exp() const;

  /**
   * @brief Set the exponent of the receive timeout.
   * @param er the exponent for the receive timeout (see L4_timeout()).
   * @see rcv_man()
   */
  void rcv_exp( Mword er );

  /**
   * @brief Get the receive timout's mantissa.
   * @return The mantissa of the receive timeout (see L4_timeout()).
   * @see rcv_exp()
   */
  Mword rcv_man() const;

  /**
   * @brief Set the mantissa of the receive timeout.
   * @param mr the mantissa of the recieve timeout (see L4_timeout()).
   * @see rcv_exp()
   */
  void rcv_man( Mword mr );

  /**
   * @brief Get tge send timeout exponent.
   * @return The exponent of the send timout (see L4_timeout()).
   * @see snd_man()
   */
  Mword snd_exp() const;

  /**
   * @brief Set the exponent of the send timeout.
   * @param es the exponent of the send timeout (see L4_timeout()).
   * @see snd_man()
   */
  void snd_exp( Mword es );

 /**
   * @brief Get the send timout's mantissa.
   * @return The mantissa of the send timeout (see L4_timeout()).
   * @see snd_exp()
   */
  Mword snd_man() const;

  /**
   * @brief Set the mantissa of the send timeout.
   * @param ms the mantissa of the send timeout (see L4_timeout()).
   * @see snd_exp()
   */
  void snd_man( Mword ms );

  /**
   * @brief Get the receive page fault timeout.
   * @return The exponent of the receive page fault timeout (see L4_timeout()).
   */
  Mword rcv_pfault() const;

  /**
   * @brief Set the receive page fault tiemout.
   * @param pr the exponent of the receive page fault timeout (see L4_timeout()).
   */
  void  rcv_pfault( Mword pr );

  /**
   * @brief Get the send page fault tiemout.
   * @return The exponent of the send page fault tiemout (see L4_timeout()).
   */
  Mword snd_pfault() const;

  /**
   * @brief Set the send page fault timeout.
   * @param ps the exponent of the send page fault timeout (see L4_timeout()).
   */
  void  snd_pfault( Mword ps );

  /**
   * @brief Get the relative receive timeout in microseconds.
   * @param clock Current value of kernel clock
   * @return The receive timeout in micro seconds.
   */
  Unsigned64 rcv_microsecs_rel (Unsigned64 clock) const;

  /**
   * @brief Get the relative send timeout in microseconds.
   * @param clock Current value of kernel clock
   * @return The send timeout in micro seconds.
   */
  Unsigned64 snd_microsecs_rel (Unsigned64 clock) const;

  /**
   * @brief Get the absolute receive timeout in microseconds.
   * @param clock Current value of kernel clock
   * @return The receive timeout in micro seconds.
   */
  Unsigned64 rcv_microsecs_abs (Unsigned64 clock, bool c) const;

  /**
   * @brief Get the absolute send timeout in microseconds.
   * @param clock Current value of kernel clock
   * @return The send timeout in micro seconds.
   */
  Unsigned64 snd_microsecs_abs (Unsigned64 clock, bool c) const;

private:
  Mword _t;
};


/**
 * @brief L4 Message Dope.
 *
 * A value of this type is returned as result code for any IPC
 * operation.
 */
class L4_msgdope
{
public:
  /**
   * @anchor error_codes
   * @brief Constants an bit masks for error codes.
   */
  enum {
    /// If this bit is set a error occurred in the send pahse.
    SEND_ERROR    =             0x10, 
    ENOT_EXISTENT =		0x10, ///< Receiver does not exist.
    RETIMEOUT     =		0x20, ///< Receive timeout.
    SETIMEOUT     =		0x30, ///< Send timeout.
    RECANCELED    =		0x40, ///< Receive canceled.
    SECANCELED    =		0x50, ///< Send canceled.
    REMAPFAILED   =		0x60, ///< Receive mapping failed.
    SEMAPFAILED   =		0x70, ///< Send mapping failed.
    RESNDPFTO     =		0x80, ///< Receive send-page-fault timeout.
    SESNDPFTO     =	 	0x90, ///< Send send-page-fault timeout.
    RERCVPFTO     =		0xA0, ///< Receive receive-page-fault timeout.
    SERCVPFTO     =		0xB0, ///< Send receive-page-fault timeout.
    REABORTED     =		0xC0, ///< Receive aborted.
    SEABORTED     =		0xD0, ///< Send aborted.
    REMSGCUT      =		0xE0, ///< Receive message cut.
    SEMSGCUT      =		0xF0, ///< Send message cut.
  };

  /**
   * @brief Create a message dope from the binary representation.
   * @param raw the binary representation of a message dope.
   */
  L4_msgdope( Mword raw = 0 );

  /**
   * @brief Create the specified message dope.
   * @param mwords the number of transfered message words.
   * @param strings the number of transferd indirect strings.
   */
  L4_msgdope( Mword mwords, Mword strings );

  /**
   * @brief Get the binary representation of this message dope.
   * @return The binary form of this message dope.
   */
  Mword raw() const;

  /**
   * @brief Message was deceited?
   * @return true if the message was deceited (the deceite bit is set).
   */
  Mword deceited() const;

  /**
   * @brief Flexpage received?
   * @return true if a flex page was received.
   */
  Mword fpage_received() const;

  /**
   * @brief Set/Clear the flex page received bit.
   * @param f if true the bit is set or cleared else.
   */
  void fpage_received( Mword f );

  /**
   * @brief Message redirected?
   * @return The state of the redirect bit.
   */
  Mword redirected() const;

  /**
   * @brief Source inside?
   * @return The state of the source-inside bit.
   */
  Mword src_inside() const;

  /**
   * @brief Was there a send error?
   * @return true if there was a send error.
   */
  Mword snd_error() const;

  /**
   * @brief Get the error code.
   * @return The error code (see @ref error_codes "Error Codes").
   */
  Mword error() const;

  /**
   * @brief Set the error code.
   * @param error the error code to be set (see @ref error_codes "Error Codes").
   * @see error_code()
   */
  void error( Mword error );

  /**
   * @brief Was there an error?
   * @return true if there was any error.
   */
  Mword has_error() const;

  /**
   * @brief Return short word explaining the error
   * @return pointer to static string
   */
  const char * str_error() const;

  /**
   * @brief Get the number of indirect strings.
   * @return The number of indirect strings.
   */
  Mword strings() const;

  /**
   * @brief Set the number of indirect strings.
   * @param s the number of indirect strings.
   */
  void strings( Mword s );

  /**
   * @brief Get the number of transfered message words.
   * @return the number of message words tranfered.
   */
  Mword mwords() const;

  /**
   * @brief Set the number of transfered message words.
   * @param w the number of message words transfered.
   */
  void mwords( Mword w );

  /**
   * @brief Error == Receive map failed?
   * @return true if error is receive map failed.
   * @see @ref error_codes "Error Codes"
   */
  Mword rcv_map_failed() const;

  /**
   * @brief Error == Send map failed?
   * @return true if error is send map failed.
   * @see @ref error_codes "Error Codes"
   */
  Mword snd_map_failed() const;

  /**
   * @brief Combine two message dopes.
   *
   * This method is for combining two message dopes, 
   * this means the binary representations of the dopes
   * are or'd together.
   *
   * @param other the other message dope to combine.
   */
  void combine( L4_msgdope other );

private:
  Mword _raw;
  enum {
    DECEITE_BIT      = 0,
    FPAGE_BIT        = 1,
    REDIRECTED_BIT   = 2,
    SRC_INSIDE_BIT   = 3,
    SND_ERROR_BIT    = 4,
    ERROR_CODE_SHIFT = 5,
    ERROR_CODE_MASK  = 0x7L << ERROR_CODE_SHIFT,
    ERROR_CODE_SIZE  = 3,
    ERROR_SHIFT      = 4,
    ERROR_MASK       = 0xfL << ERROR_SHIFT,
    ERROR_SIZE       = 4,
    STRINGS_SHIFT    = ERROR_CODE_SHIFT + ERROR_CODE_SIZE,
    STRINGS_MASK     = 0x1fL << STRINGS_SHIFT,
    STRINGS_SIZE     = 5,
    MWORDS_SHIFT     = STRINGS_SHIFT + STRINGS_SIZE,
    MWORDS_MASK      = 0x7ffffL << MWORDS_SHIFT,
    MWORDS_SIZE      = 19,

  };

};


/**
 * @brief L4 String Dope.
 *
 * Descriptor of indirect strings in V2 and X0 long IPC messages.
 */
class L4_str_dope 
{
public:
  /// Length of the string to send.
  Mword snd_size;

  /// Address of the string to send.
  Unsigned8 *snd_str;

  /// Size of the receive string buffer.
  Mword rcv_size;

  /// Address of the receive string buffer.
  Unsigned8 *rcv_str;
};


/**
 * @brief L4 Scheduling Parameters.
 */
class L4_sched_param
{
public:

  /**
   * @brief Create scheduling params from the binary representation.
   */
  L4_sched_param( Mword raw = 0 );

  /**
   * @brief Are the params valid?
   * @return true if the raw prepresentation is not (Mword)-1.
   */ 
  Mword is_valid() const;

  /**
   * @brief Get the priority.
   * @return The static priority.
   */
  Mword prio() const;

  /**
   * @brief Set the priority.
   * @param p the new static priority.
   */
  void prio( Mword p );

  /**
   * @brief Get the small address space number.
   * @return The small address space number.
   */
  Mword small() const;

  /**
   * @brief Set the small address space number.
   * @param s the new small address space number.
   */
  void small( Mword s );

  /**
   * @brief Get the mode number.
   * @return The mode number.
   */
  Mword mode() const;

  /**
   * @brief Set the returned thread_state.
   * @param s the new returned thread_state.
   */
  void thread_state( Mword s );

  /**
   * @brief Set the timeslice in micro seconds.
   * @param t the new timeslice in micro seconds.
   * @see time_exp(), time_man()
   */
  void time( Unsigned64 t );

  /**
   * @brief Get the timeslice in micro seconds.
   * @return The timeslice in micro seconds.
   * @see time_exp(), time_man()
   */
  Unsigned64 time();

  /**
   * @brief Get the CPU time exponent.
   * @return The exponent of the CPU time.
   * @see time()
   */
  Mword time_exp() const;

  /**
   * @brief Set the CPU time exponent.
   * @param e the new CPU time exponent.
   * @see time()
   */
  void time_exp( Mword e );

  /**
   * @brief Get the CPU time mantissa.
   * @return The mantissa of the CPU time.
   * @see time()
   */
  Mword time_man() const;

  /**
   * @brief Set the mantissa of the CPU time.
   * @param m the new mantissa of the CPU time.
   * @see time()
   */
  void time_man( Mword m );

  /**
   * @brief Get the raw representation of the params.
   * @return The binary representation of the params.
   */
  Mword raw() const;

private:

  Mword _raw;

  enum {
    PRIO_SHIFT     = 0,
    PRIO_MASK      = 0x0ff,
    PRIO_SIZE      = 8,
    SMALL_SHIFT    = PRIO_SIZE + PRIO_SHIFT,
    SMALL_SIZE     = 8,
    SMALL_MASK     = ((1UL << SMALL_SIZE)-1) << SMALL_SHIFT,
    MODE_SHIFT     = SMALL_SHIFT + SMALL_SIZE,
    MODE_SIZE      = 4,
    MODE_MASK      = ((1UL << MODE_SIZE)-1) << MODE_SHIFT,
    TIME_EXP_SHIFT = MODE_SHIFT + MODE_SIZE, 
    TIME_EXP_SIZE  = 4,
    TIME_EXP_MASK  = ((1UL << TIME_EXP_SIZE)-1) << TIME_EXP_SHIFT,
    TIME_MAN_SHIFT = TIME_EXP_SIZE + TIME_EXP_SHIFT,
    TIME_MAN_SIZE  = 8,
    TIME_MAN_MASK  = ((1UL << TIME_MAN_SIZE)-1) << TIME_MAN_SHIFT,
  };

};


IMPLEMENTATION:

IMPLEMENT inline 
L4_snd_desc::L4_snd_desc( Mword w )
  : _d(w)
{}
  
IMPLEMENT inline 
Mword L4_snd_desc::deceite() const
{
  return _d & 1;
}

IMPLEMENT inline 
Mword L4_snd_desc::map() const
{
  return _d & 2;
}

IMPLEMENT inline 
Mword L4_snd_desc::is_long_ipc() const
{
  return _d & ~1;
}

IMPLEMENT inline
Mword L4_snd_desc::is_register_ipc() const
{
  return (_d & ~1) == 0;
}

IMPLEMENT inline 
Mword L4_snd_desc::has_send() const
{
  return _d != (Mword)-1;
}

IMPLEMENT inline 
void *L4_snd_desc::msg() const
{
  return (void*)(_d & ~3);
}

PUBLIC inline
Mword L4_snd_desc::raw() const
{
  return _d;
}


IMPLEMENT inline
L4_rcv_desc L4_rcv_desc::short_fpage( L4_fpage fp )
{
  return L4_rcv_desc( 2 /*rmap*/ | (fp.raw() & ~3) );
}


IMPLEMENT inline 
L4_rcv_desc::L4_rcv_desc( Mword w = (Mword)-1 )
  : _d(w)
{}

IMPLEMENT inline 
Mword L4_rcv_desc::open_wait() const
{
  return _d & 1;
}

IMPLEMENT inline 
Mword L4_rcv_desc::rmap() const
{
  return _d & 2;
}

IMPLEMENT inline 
void *L4_rcv_desc::msg() const
{
  return (void*)(_d & ~3);
}

IMPLEMENT inline 
L4_fpage L4_rcv_desc::fpage() const
{
  return L4_fpage( _d & ~3 );
}


IMPLEMENT inline
Mword L4_rcv_desc::is_register_ipc() const
{
  return (_d & ~1) == 0;
}
  
IMPLEMENT inline 
Mword L4_rcv_desc::has_receive() const
{
  return _d != (Mword)-1;
}

/// for debugging (jdb)
PUBLIC inline
Mword L4_rcv_desc::raw() const
{
  return _d;
}

IMPLEMENT inline 
L4_timeout::L4_timeout( Mword t )
  : _t(t)
{}

IMPLEMENT inline
L4_timeout::L4_timeout( Mword snd_man, Mword snd_exp, 
			Mword rcv_man, Mword rcv_exp, 
			Mword snd_pflt, Mword rcv_pflt )
  : _t( ((snd_man << SND_MAN_SHIFT) & SND_MAN_MASK) |
	((snd_exp << SND_EXP_SHIFT) & SND_EXP_MASK) |
	((rcv_man << RCV_MAN_SHIFT) & RCV_MAN_MASK) |
	((rcv_exp << RCV_EXP_SHIFT) & RCV_EXP_MASK) |
	((rcv_pflt << RCV_PFAULT_SHIFT) & RCV_PFAULT_MASK) |
	((snd_pflt << SND_PFAULT_SHIFT) & SND_PFAULT_MASK) )
{}
 
IMPLEMENT inline
Unsigned64 L4_timeout::raw()
{
  return _t;
}

IMPLEMENT inline
Mword L4_timeout::rcv_exp() const
{
  return (_t & RCV_EXP_MASK) >> RCV_EXP_SHIFT;
}

IMPLEMENT inline
void  L4_timeout::rcv_exp( Mword w )
{
  _t = (_t & ~RCV_EXP_MASK) | ((w << RCV_EXP_SHIFT) & RCV_EXP_MASK);
}
 
IMPLEMENT inline
Mword L4_timeout::snd_exp() const
{
  return (_t & SND_EXP_MASK) >> SND_EXP_SHIFT;
}

IMPLEMENT inline
void  L4_timeout::snd_exp( Mword w )
{
  _t = (_t & ~SND_EXP_MASK) | ((w << SND_EXP_SHIFT) & SND_EXP_MASK);
}

IMPLEMENT inline
Mword L4_timeout::rcv_pfault() const
{
  return (_t & RCV_PFAULT_MASK) >> RCV_PFAULT_SHIFT;
}


IMPLEMENT inline
void  L4_timeout::rcv_pfault( Mword w ) 
{
  _t = (_t & ~RCV_PFAULT_MASK) | ((w << RCV_PFAULT_SHIFT) & RCV_PFAULT_MASK);
}

IMPLEMENT inline
Mword L4_timeout::snd_pfault() const
{
  return (_t & SND_PFAULT_MASK) >> SND_PFAULT_SHIFT;
}

IMPLEMENT inline
void  L4_timeout::snd_pfault( Mword w )
{
  _t = (_t & ~SND_PFAULT_MASK) | ((w << SND_PFAULT_SHIFT) & SND_PFAULT_MASK);
}

IMPLEMENT inline
Mword L4_timeout::rcv_man() const
{
  return (_t & RCV_MAN_MASK) >> RCV_MAN_SHIFT;
}

IMPLEMENT inline
void  L4_timeout::rcv_man( Mword w )
{
  _t = (_t & ~RCV_MAN_MASK) | ((w << RCV_MAN_SHIFT) & RCV_MAN_MASK);
}

IMPLEMENT inline
Mword L4_timeout::snd_man() const
{
  return (_t & SND_MAN_MASK) >> SND_MAN_SHIFT;
}

IMPLEMENT inline
void L4_timeout::snd_man( Mword w )
{
  _t = (_t & ~SND_MAN_MASK) | ((w << SND_MAN_SHIFT) & SND_MAN_MASK);
}

IMPLEMENT inline
Unsigned64 L4_timeout::rcv_microsecs_rel (Unsigned64 clock) const
{
  return clock + ((Unsigned64)(rcv_man()) << ((15 - rcv_exp()) << 1));
}

IMPLEMENT inline
Unsigned64 L4_timeout::snd_microsecs_rel (Unsigned64 clock) const
{
  return clock + ((Unsigned64)(snd_man()) << ((15 - snd_exp()) << 1));
}

IMPLEMENT inline
Unsigned64 L4_timeout::rcv_microsecs_abs (Unsigned64 clock, bool c) const
{
  Mword e = 15 - rcv_exp();
  Unsigned64 timeout = clock & ~((1 << e + 8) - 1) | rcv_man() << e;

  if (((clock >> e + 8) & 1) != c)
    timeout += 1 << e + 8;

  if (timeout > clock + (1 << e + 8))
    timeout -= 1 << e + 9;

  return timeout;
}

IMPLEMENT inline
Unsigned64 L4_timeout::snd_microsecs_abs (Unsigned64 clock, bool c) const
{
  Mword e = 15 - snd_exp();
  Unsigned64 timeout = clock & ~((1 << e + 8) - 1) | snd_man() << e;

  if (((clock >> e + 8) & 1) != c)
    timeout += 1 << e + 8;

  if (timeout > clock + (1 << e + 8))
    timeout -= 1 << e + 9;

  return timeout;
}

IMPLEMENT inline
L4_msgdope::L4_msgdope( Mword mwords, Mword strings )
  : _raw( ((strings << STRINGS_SHIFT) & STRINGS_MASK)
	  |((mwords << MWORDS_SHIFT) & MWORDS_MASK) )
{}

IMPLEMENT inline
L4_msgdope::L4_msgdope( Mword raw )
  : _raw(raw)
{}

IMPLEMENT inline
Mword L4_msgdope::raw() const
{
  return _raw;
}

IMPLEMENT inline
Mword L4_msgdope::deceited() const
{
  return _raw & (1 << DECEITE_BIT);
}

IMPLEMENT inline
Mword L4_msgdope::fpage_received() const
{
  return _raw & (1 << FPAGE_BIT);
}

IMPLEMENT inline
Mword L4_msgdope::redirected() const
{
  return _raw & (1 << REDIRECTED_BIT);
}

IMPLEMENT inline
Mword L4_msgdope::src_inside() const
{
  return _raw & (1 << SRC_INSIDE_BIT);
}

IMPLEMENT inline
Mword L4_msgdope::snd_error() const
{
  return _raw & (1 << SND_ERROR_BIT);
}

IMPLEMENT inline
Mword L4_msgdope::strings() const
{
  return (_raw & STRINGS_MASK) >> STRINGS_SHIFT;
}

IMPLEMENT inline
Mword L4_msgdope::mwords() const
{
  return (_raw & MWORDS_MASK) >> MWORDS_SHIFT;
}

IMPLEMENT inline
void L4_msgdope::mwords( Mword w )
{
  _raw = (_raw & ~MWORDS_MASK) | ((w<<MWORDS_SHIFT) & MWORDS_MASK);
}

IMPLEMENT inline
void L4_msgdope::strings( Mword w )
{
  _raw = (_raw & ~STRINGS_MASK) | ((w<<STRINGS_SHIFT) & STRINGS_MASK);
}


IMPLEMENT inline
Mword L4_msgdope::has_error() const
{
  return _raw & ERROR_MASK;
}

IMPLEMENT inline
Mword L4_msgdope::error() const
{
  return _raw & ERROR_MASK;
}

// This function must not be inlined because in that case the str[] array
// would be inserted as often as an L4_msgdope type is used anywhere. It
// is only an issue with gcc 2.95, newer gcc version handle this correctly.
IMPLEMENT
char const * L4_msgdope::str_error() const
{
  static char const * const str[] =
    {
      "OK",
      "ENOT_EXISTENT", "RETIMEOUT", "SETIMEOUT", "RECANCELED", "SECANCELED",
      "REMAPFAILED", "SEMAPFAILED", "RESNDPFTO", "SESNDPFTO",  "RERCVPFTO",
      "SERCVPFTO", "REABORTED", "SEABORTED", "REMSGCUT", "SEMSGCUT"
    };
			    
  return str[error() >> 4];
}

IMPLEMENT inline
Mword L4_msgdope::rcv_map_failed() const
{
  return (_raw & ERROR_MASK) == REMAPFAILED;
}

IMPLEMENT inline
Mword L4_msgdope::snd_map_failed() const
{
 return (_raw & ERROR_MASK) == SEMAPFAILED;
}

IMPLEMENT inline
void L4_msgdope::error( Mword e )
{
  _raw = (_raw & ~ERROR_MASK) | (e & ERROR_MASK);
}

IMPLEMENT inline
void L4_msgdope::fpage_received( Mword w )
{
  if(w)
    _raw |= (1L<<FPAGE_BIT);
  else
    _raw &= ~(1L<<FPAGE_BIT);
}

IMPLEMENT inline
void L4_msgdope::combine( L4_msgdope o )
{
  _raw = (_raw & ~ERROR_MASK) | o._raw;
}



IMPLEMENT inline
L4_sched_param::L4_sched_param( Mword raw )
  : _raw(raw)
{}

IMPLEMENT inline
Mword L4_sched_param::prio() const
{
  return (_raw & PRIO_MASK) >> PRIO_SHIFT;
}

IMPLEMENT inline
void L4_sched_param::prio( Mword p )
{
  _raw = (_raw & ~PRIO_MASK) | ((p<<PRIO_SHIFT) & PRIO_MASK);
}

IMPLEMENT inline
Mword L4_sched_param::small() const
{
  return (_raw & SMALL_MASK) >> SMALL_SHIFT;
}

IMPLEMENT inline
void L4_sched_param::small( Mword s )
{
  _raw = (_raw & ~SMALL_MASK) | ((s<<SMALL_SHIFT) & SMALL_MASK);
}

IMPLEMENT inline
Mword L4_sched_param::time_exp() const
{
  return (_raw & TIME_EXP_MASK) >> TIME_EXP_SHIFT;
}

IMPLEMENT inline
void L4_sched_param::time_exp( Mword e )
{
  _raw = (_raw & ~TIME_EXP_MASK) | ((e<<TIME_EXP_SHIFT) & TIME_EXP_MASK);
}

IMPLEMENT inline
Mword L4_sched_param::time_man() const
{
  return (_raw & TIME_MAN_MASK) >> TIME_MAN_SHIFT;
}

IMPLEMENT inline
void L4_sched_param::time_man( Mword m )
{
  _raw = (_raw & ~TIME_MAN_MASK) | ((m<<TIME_MAN_SHIFT) & TIME_MAN_MASK);
}

IMPLEMENT inline
Mword L4_sched_param::mode() const
{
  return (_raw & MODE_MASK) >> MODE_SHIFT;
}

IMPLEMENT inline
void L4_sched_param::thread_state( Mword e )
{
  _raw = (_raw & ~MODE_MASK) | ((e << MODE_SHIFT) & MODE_MASK);
}

IMPLEMENT inline
Mword L4_sched_param::raw() const
{
  return _raw;
}

IMPLEMENT inline
Mword L4_sched_param::is_valid() const
{
  return _raw != (Mword)-1;
}

IMPLEMENT inline
void L4_sched_param::time( Unsigned64 t )
{
  Mword exp = 15;
  while (t > 255)
    {
      t >>= 2;
      exp--;
    }

  time_exp(exp);
  time_man(t);
}

IMPLEMENT inline
Unsigned64 L4_sched_param::time()
{
  if(time_exp())
    {
      if(time_man())
	return (Unsigned64)time_man() << ((15-time_exp()) << 1);
      else
	return 0;
    }
  else
    return (Unsigned64)-1;
}
