/*
 * arch. independent L4 Types
 */

INTERFACE:

#include "types.h"

class Utcb;
typedef Address Local_id;

/// Type for task numbers.
typedef Unsigned32 Task_num;

/// Type for thread numbers (task local).
typedef Unsigned32 LThread_num;

/// Type for global thread numbers.
typedef Unsigned32 GThread_num;

/**
 * L4 unique ID.
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
  /// The standard IDs.
  enum
  {
    Invalid = 0xffffffff,	///< The Invalid ID
    Nil     = 0x00000000,	///< The Nil ID
  };

  /**
   * Create an uninitialized UID.
   */
  L4_uid ();

  /**
   * Create an UID from an virtual address.
   */
  L4_uid (void *addr, Address tcb_base, Address tcb_size);

  /**
   * Extract the version part of the UID.
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
   * Is this a NIL ID?
   * @return 0 if this is not the NIL ID, not 0 else.
   */
  Mword is_nil() const;

  /**
   * Is this the INVALID ID?
   * @return 0 if this is not the INVALID ID, not 0 else.
   */
  Mword is_invalid() const;
  bool is_valid() const;

  /// Get the L4 UID for the given IRQ.
  static L4_uid irq( unsigned irq );

  /// Is this a IRQ ID?
  Mword is_irq() const;

  /**
   * Get the IRQ number.
   * @pre is_irq() must return true, or the result is unpredictable.
   * @return the IRQ number for the L4 IRQ ID.
   */
  Mword irq() const;

  /// Test for equality.
  bool operator == ( L4_uid const &o ) const;

  /// Test for inequality.
  bool operator != ( L4_uid const &o ) const
  { return ! operator == (o); }

  /**
   * Extract the task number.
   */
  Task_num task() const;

  /**
   * Set the task number.
   */
  void task (Task_num);

  /*
   * Get the task ID, the local thread-number is set to 0.
   */
  L4_uid task_id() const;

  /**
   * Get the maximum number of threads per task.
   */
  static unsigned threads_per_task();

  /**
   * Get number of threads in the system.
   * This method only works for v2 and x0 ABI. To get the max_threads
   * value ABI-independently, use Config::max_threads() instead.
   */
  static Mword max_threads();
};

typedef L4_uid Global_id;

/**
 * L4 send descriptor.
 */
class L4_snd_desc
{
public:

  /**
   * Create a send descriptor from its binary representation.
   */
  L4_snd_desc( Mword w = (Mword)-1 );

  /**
   * Deceite bit set?
   */
  Mword deceite() const;

  /**
   * Map bit set?
   */
  Mword map() const;

  /**
   * Get the message base address.
   */
  void *msg() const;

  /**
   * Is the message a long IPC?
   */
  Mword is_long_ipc() const;

  /**
   * Send a register only message?
   * @return true, if a register only message shall be sended.
   */
  Mword is_register_ipc() const;

  /**
   * has the IPC a send part (descriptor != -1).
   */
  Mword has_snd() const;

private:
  Mword _d;
};


/**
 * A L4 flex page.
 *
 * A flex page represents a size aligned
 * region of an address space.
 */
class L4_fpage
{
public:
  typedef Mword Raw;
  typedef Raw Cache_type;

  enum Cacheing
  {
    Uncached    = 0x200,
    Cached      = 0x600,
    Buffered    = 0xa00,
    No_change   = 0x000,
    Caching_opt = 0x200
  };

  /**
   * Create a flexpage with the given parameters.
   * @param grant if not zero the grant bit is to be set.
   * @param write if not zero the write bit is to be set.
   * @param order the size of the flex page is 2^order.
   * @param page the base address of the flex page.
   */
  L4_fpage (Mword grant, Mword write, Mword order, Mword page, 
      short cache = 0);

  /**
   * Create a flexpage with the given parameters.
   *
   * This constructor is ABI-independent, thus it sets the
   * ABI-independent fpage attributes only.  The other bits
   * are set to 0.
   *
   * @param order the size of the flex page is 2^order.
   * @param page the base address of the flex page.
   *
   */

  L4_fpage( Mword order, Mword page );

  /**
   * Is the grant bit set?
   * @return the state of the grant bit.
   */
  Mword grant() const;

  /**
   * Set the grant bit according to g.  Overwrites state managed 
   * by status() members.
   * @param g if not zero the grant bit is to be set.
   */
  void grant (Mword g);
  /**
   * Is the write bit set?
   *
   * @return the state of the write bit.
   */
  Mword write() const;

  /**
   * Set the write bit according to w.  Overwrites state managed 
   * by status() members.
   *
   * @param w if not zero the write bit is to be set.
   */
  void write( Mword w );

  /**
   * Get the size part of the flex page.
   * @return the order of the flex page (size part).
   */
  Mword size() const;

  /**
   * Set the size part of the flex page.
   * @param order the size is 2^order.
   */
  void size( Mword order );

  /**
   * Get the base address of the flex page.
   * @return the base address.
   */
  Mword page() const;

  /**
   * Set the base address of the flex page.
   * @param base the flex page's base address.
   */
  void page( Mword base );

  /**
   * Is the flex page the whole address space?
   * @return not zero, if the flex page covers the
   *   whole address space.
   */
  Mword is_whole_space() const;

  /**
   * Is the flex page valid?
   * @return not zero if the flex page
   *    contains a value other than 0.
   */
  Mword is_valid() const;

  /**
   * Get the status flags (Accessed, Dirty, Executed) of the flexpage
   * (ABI extension).
   * Only meaningful after calling L4_fpage::status (Mword status).
   * @return the status flags.
   */
  Mword status() const;

  /**
   * Set the flex page's status flags (ABI extension).
   * @param status the flex page's base status flags.  Overwrites
   *               state managed by grant() and write() members.
   */
  void status (Mword status);

  /**
   * Create a flex page from the binary representation.
   * @param w the binary representation.
   */
  L4_fpage(Raw w = 0);

  /**
   * Get the binary representation of the flex page.
   * @return this flex page in binary representation.
   */
  Raw raw() const;

  bool is_all_spaces() const;

private:

  Raw _raw;

  enum {
    /* +- bitsize-12 + 11-9 + 8 +- 7-2 +- 1 -+- 0 -+
     * | page number |   C  | X | size | W/D | G/A |
     * +-------------+------+---+------+-----+-----+ */
    Grant_bit        = 0, ///< G (Grant)
    Write_bit        = 1, ///< W (Write)
    Size_shift       = 2,
    Page_shift       = 0,

    // Extension: bits for returning a flexpage's access status.
    // (Make sure these flags do not overlap any significant fpage
    // bits -- but overlapping the Grant and Write bits used in some
    // APIs is OK.)
    Referenced_bit   = 0, ///< A (Referenced)
    Dirty_bit        = 1, ///< D (Dirty)
    // XXX Executed_bit = 8, ///< X (Executed)
  };

public:
  enum {
    Referenced       = 1 << Referenced_bit,
    Dirty            = 1 << Dirty_bit,
    // XXX Executed  = 1 << Executed_bit,
    Status_mask      = (Referenced | Dirty /* | Executed */ ),
  };
};


/**
 * L4 receive descriptor.
 */
class L4_rcv_desc
{
public:

  /**
   * Create a receive descriptor for receiving the given flex page.
   *
   * The descriptor is for receiving the given flex page by an register only
   * message.
   * @param fp the receive flex page.
   * @return An L4 receive descriptor for receiving the given flex page
   *         as register only message.
   *
   */
  static L4_rcv_desc short_fpage (L4_fpage fp);

  /**
   * Create a L4 receive descriptor from it's binary representation.
   */
  L4_rcv_desc( Mword w = (Mword)-1 );

  /**
   * Open wait?
   * @return the open wait bit.
   */
  Mword open_wait() const;

  /**
   * Wait for a register map message?
   * @return whether a register map receive is expected.
   */
  Mword rmap() const;

  /**
   * Get a pointer to the receive buffer.
   * @pre The is_register_ipc() method must return false.
   * @return the address of the receive buffer.
   */
  void *msg() const;

  /**
   * Get the receive flex page.
   * @pre rmap() must return true.
   * @return the receive flex page.
   */
  L4_fpage fpage() const;

  /**
   * Receive a register only message?
   * @return true, if a register only message shall be received.
   */
  Mword is_register_ipc() const;

  /**
   * Is there a receive part?
   * @return whether there is a receive expected or not.
   */
  Mword has_receive() const;

private:
  Mword _d;

};


/**
 * L4 IPC error code
 */
class Ipc_err
{
public:
  /**
   * Create a error code from the binary representation.
   * @param raw the binary representation of the error code.
   */
  Ipc_err( Mword raw = 0 );

  /**
   * Get the binary representation.
   * @return The binary form of this error code.
   */
  Mword raw() const;

  /**
   * Message was deceited?
   * @return true if the message was deceited (the deceite bit is set).
   */
  Mword deceited() const;

  /**
   * Flexpage received?
   * @return true if a flex page was received.
   */
  Mword fpage_received() const;

  /**
   * Set/Clear the flex page received bit.
   * @param f if true the bit is set or cleared else.
   */
  void fpage_received( Mword f );

  /**
   * Message redirected?
   * @return The state of the redirect bit.
   */
  Mword redirected() const;

  /**
   * Source inside?
   * @return The state of the source-inside bit.
   */
  Mword src_inside() const;

  /**
   * Was there a send error?
   * @return true if there was a send error.
   */
  Mword snd_error() const;

  /**
   * Get the error code.
   * @return The error code (see @ref error_codes "Error Codes").
   */
  Mword error() const;

  /**
   * Set the error code.
   * @param error the error code to be set
   *        (see @ref error_codes "Error Codes").
   * @see error_code()
   */
  void error( Mword error );

  /**
   * Was there an error?
   * @return true if there was any error.
   */
  Mword has_error() const;

  /**
   * Return short word explaining the error
   * @return pointer to static string
   */
  const char * str_error() const;

  /**
   * Error == Receive map failed?
   * @return true if error is receive map failed.
   * @see @ref error_codes "Error Codes"
   */
  Mword rcv_map_failed() const;

  /**
   * Error == Send map failed?
   * @return true if error is send map failed.
   * @see @ref error_codes "Error Codes"
   */
  Mword snd_map_failed() const;

  /**
   * Combine two error codes.
   *
   * This method is for combining two error codes,
   * this means the binary representations of the codes
   * are or'd together.
   *
   * @param other the other error code to combine.
   */
  void combine( Ipc_err other );

  /**
   * @anchor error_codes
   * Constants and bit masks for error codes.
   */
  enum
    {
      /// If this bit is set an error occurred in the send phase.
      Send_error    =		0x10,
      Enot_existent =		0x10, ///< Receiver does not exist.
      Retimeout     =		0x20, ///< Receive timeout.
      Setimeout     =		0x30, ///< Send timeout.
      Recanceled    =		0x40, ///< Receive canceled.
      Secanceled    =		0x50, ///< Send canceled.
      Remapfailed   =		0x60, ///< Receive mapping failed.
      Semapfailed   =		0x70, ///< Send mapping failed.
      Resndpfto     =		0x80, ///< Receive snd-page-fault timeout.
      Sesndpfto     =		0x90, ///< Send snd-page-fault timeout.
      Rercvpfto     =		0xA0, ///< Receive receive-page-fault timeout.
      Sercvpfto     =		0xB0, ///< Send receive-page-fault timeout.
      Reaborted     =		0xC0, ///< Receive aborted.
      Seaborted     =		0xD0, ///< Send aborted.
      Remsgcut      =		0xE0, ///< Receive message cut.
      Semsgcut      =		0xF0, ///< Send message cut.
    };

private:
  enum
    {
      Deceite_bit      = 0,
      Fpage_bit        = 1,
      Redirected_bit   = 2,
      Src_inside_bit   = 3,
      Snd_error_bit    = 4,
      Error_code_shift = 5,
      Error_code_mask  = 0x7L << Error_code_shift,
      Error_code_size  = 3,
      Error_shift      = 4,
      Error_mask       = 0xfL << Error_shift,
      Error_size       = 4,
      Cc_mask          = 0xFF,
    };

  Mword _raw;
};


/**
 * L4 Message Dope.
 *
 * A value of this type is returned as result code for any IPC
 * operation.
 */
class L4_msgdope
{
public:
  /**
   * @anchor error_codes
   * Constants an bit masks for error codes.
   */
  enum {
    /// If this bit is set a error occurred in the send phase.
    Send_error    =             0x10, 
    Enot_existent =		0x10, ///< Receiver does not exist.
    Retimeout     =		0x20, ///< Receive timeout.
    Setimeout     =		0x30, ///< Send timeout.
    Recanceled    =		0x40, ///< Receive canceled.
    Secanceled    =		0x50, ///< Send canceled.
    Remapfailed   =		0x60, ///< Receive mapping failed.
    Semapfailed   =		0x70, ///< Send mapping failed.
    Resndpfto     =		0x80, ///< Receive snd-page-fault timeout.
    Sesndpfto     =		0x90, ///< Send snd-page-fault timeout.
    Rercvpfto     =		0xA0, ///< Receive receive-page-fault timeout.
    Sercvpfto     =		0xB0, ///< Send receive-page-fault timeout.
    Reaborted     =		0xC0, ///< Receive aborted.
    Seaborted     =		0xD0, ///< Send aborted.
    Remsgcut      =		0xE0, ///< Receive message cut.
    Semsgcut      =		0xF0, ///< Send message cut.
  };

  /**
   * Create a message dope from the binary representation.
   * @param raw the binary representation of a message dope.
   */
  L4_msgdope( Mword raw = 0 );

  /**
   * Create the specified message dope.
   * @param mwords the number of transfered message words.
   * @param strings the number of transferd indirect strings.
   */
  L4_msgdope( L4_snd_desc snd_desc, Mword mwords, Mword strings );

  /**
   * Type conversion constructor
   * Constructs an L4_msgdope from an error code
   * @param e the error code
   */
  L4_msgdope (Ipc_err e);

  /**
   * Get the binary representation of this message dope.
   * @return The binary form of this message dope.
   */
  Mword raw() const;

  /**
   * Get the binary representation with the error bits masked out
   */
  Mword raw_dope() const;

  /**
   * Message was deceited?
   * @return true if the message was deceited (the deceite bit is set).
   */
  Mword deceited() const;

  /**
   * Flexpage received?
   * @return true if a flex page was received.
   */
  Mword fpage_received() const;

  /**
   * Set/Clear the flex page received bit.
   * @param f if true the bit is set or cleared else.
   */
  void fpage_received( Mword f );

  /**
   * Message redirected?
   * @return The state of the redirect bit.
   */
  Mword redirected() const;

  /**
   * Source inside?
   * @return The state of the source-inside bit.
   */
  Mword src_inside() const;

  /**
   * Was there a send error?
   * @return true if there was a send error.
   */
  Mword snd_error() const;

  /**
   * Get the error code.
   * @return The error code (see @ref error_codes "Error Codes").
   */
  Mword error() const;

  /**
   * Set the error code.
   * @param error the error code to be set
   *        (see @ref error_codes "Error Codes").
   * @see error_code()
   */
  void error( Mword error );

  /**
   * Was there an error?
   * @return true if there was any error.
   */
  Mword has_error() const;

  /**
   * Return short word explaining the error
   * @return pointer to static string
   */
  const char * str_error() const;

  /**
   * Get the number of indirect strings.
   * @return The number of indirect strings.
   */
  Mword strings() const;

  /**
   * Set the number of indirect strings.
   * @param s the number of indirect strings.
   */
  void strings( Mword s );

  /**
   * Get the number of transfered message words.
   * @return the number of message words tranfered.
   */
  Mword mwords() const;

  /**
   * Set the number of transfered message words.
   * @param w the number of message words transfered.
   */
  void mwords( Mword w );

  /**
   * Error == Receive map failed?
   * @return true if error is receive map failed.
   * @see @ref error_codes "Error Codes"
   */
  Mword rcv_map_failed() const;

  /**
   * Error == Send map failed?
   * @return true if error is send map failed.
   * @see @ref error_codes "Error Codes"
   */
  Mword snd_map_failed() const;

  /**
   * Combine two message dopes.
   *
   * This method is for combining two message dopes,
   * this means the binary representations of the dopes
   * are or'd together.
   *
   * @param other the other message dope to combine.
   */
  void combine( L4_msgdope other );

  /**
   * Combine a message dope with an IPC error code.
   *
   * @param e the error code.
   */
  void combine( Ipc_err e );

private:
  Mword _raw;
  enum {
    Deceite_bit      = 0,
    Fpage_bit        = 1,
    Redirected_bit   = 2,
    Src_inside_bit   = 3,
    Snd_error_bit    = 4,
    Error_code_shift = 5,
    Error_code_mask  = 0x7L << Error_code_shift,
    Error_code_size  = 3,
    Error_shift      = 4,
    Error_mask       = 0xfL << Error_shift,
    Error_size       = 4,
    Strings_shift    = Error_code_shift + Error_code_size,
    Strings_mask     = 0x1fL << Strings_shift,
    Strings_size     = 5,
    Mwords_shift     = Strings_shift + Strings_size,
    Mwords_mask      = 0x7ffffL << Mwords_shift,
    Mwords_size      = 19,
    Dope_mask	     = Strings_mask | Mwords_mask,
  };

};


/**
 * L4 String Dope.
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


class L4_msg_tag
{
public:
  enum Flags
  {
    Transfer_fpu = 0x8000,
  };
  
  enum Protocol
  {
    Label_irq = -1L,
    Label_page_fault = -2L,
    Label_preemption = -3L,
    Label_sys_exception = -4L,
    Label_exception  = -5L,
    Label_sigma0 = -6L,
    Label_io_page_fault = -8L,
    Label_cap_fault = -9L,
  };
private:
  Mword _tag;
};

/**
 * L4 timeouts data type.
 */
class L4_timeout
{
public:
  /// Typical timout constants.
  enum {
    Never = 0, ///< Never time out.
  };

  /**
   * Create the specified timeout.
   * @param man mantissa of the send timeout.
   * @param exp exponent of the send timeout
   *        (exp=0: infinite timeout,
   *        exp>0: t=2^(exp)*man,
   *        man=0 & exp!=0: t=0).
   */
  L4_timeout (Mword man, Mword exp);
  L4_timeout (Mword man, Mword exp, bool clock);

  /**
   * Create a timeout from it's binary representation.
   * @param t the binary timeout value.
   */
  L4_timeout( unsigned short t = 0 );

  /**
   * Get the binary representation of the timeout.
   * @return The timeout as binary representation.
   */
  unsigned short raw() const;

  /**
   * Get the receive exponent.
   * @return The exponent of the receive timeout.
   * @see rcv_man()
   */
  Mword exp() const;

  /**
   * Set the exponent of the receive timeout.
   * @param er the exponent for the receive timeout (see L4_timeout()).
   * @see rcv_man()
   */
  void exp( Mword er );

  /**
   * Get the receive timout's mantissa.
   * @return The mantissa of the receive timeout (see L4_timeout()).
   * @see rcv_exp()
   */
  Mword man() const;

  /**
   * Set the mantissa of the receive timeout.
   * @param mr the mantissa of the recieve timeout (see L4_timeout()).
   * @see rcv_exp()
   */
  void man( Mword mr );

  /**
   * Get the relative receive timeout in microseconds.
   * @param clock Current value of kernel clock
   * @return The receive timeout in micro seconds.
   */
  Unsigned64 microsecs_rel (Unsigned64 clock) const;

  /**
   * Get the absolute receive timeout in microseconds.
   * @param clock Current value of kernel clock
   * @return The receive timeout in micro seconds.
   */
  Unsigned64 microsecs_abs (Unsigned64 clock) const;

private:
  enum
    {
      Clock_mask   = 0x0400,
      Abs_mask     = 0x8000,

      Exp_mask     = 0x7c00,
      Exp_shift    = 10,

      Man_mask     = 0x3ff,
      Man_shift    = 0,
    };

  unsigned short _t;
} __attribute__((packed));

struct L4_timeout_pair
{
  L4_timeout rcv;
  L4_timeout snd;

  L4_timeout_pair(L4_timeout const &rcv, L4_timeout const &snd)
    : rcv(rcv), snd(snd) {}

  L4_timeout_pair(unsigned long v) : rcv(v), snd(v >> 16) {}
};


/**
 * L4 Scheduling Parameters.
 */
class L4_sched_param
{
public:

  /**
   * Create scheduling params from the binary representation.
   */
  L4_sched_param( Mword raw = 0 );

  /**
   * Are the params valid?
   * @return true if the raw prepresentation is not (Mword)-1.
   */
  Mword is_valid() const;

  /**
   * Get the priority.
   * @return The static priority.
   */
  Mword prio() const;

  /**
   * Set the priority.
   * @param p the new static priority.
   */
  void prio( Mword p );

  /**
   * Get the small address space number.
   * @return The small address space number.
   */
  Mword small() const;

  /**
   * Set the small address space number.
   * @param s the new small address space number.
   */
  void small( Mword s );

  /**
   * Get the mode number.
   * @return The mode number.
   */
  Mword mode() const;

  /**
   * Set the returned thread_state.
   * @param s the new returned thread_state.
   */
  void thread_state( Mword s );

  /**
   * Set the timeslice in micro seconds.
   * @param t the new timeslice in micro seconds.
   * @see time_exp(), time_man()
   */
  void time( Unsigned64 t );

  /**
   * Get the timeslice in micro seconds.
   * @return The timeslice in micro seconds.
   * @see time_exp(), time_man()
   */
  Unsigned64 time();

  /**
   * Get the CPU time exponent.
   * @return The exponent of the CPU time.
   * @see time()
   */
  Mword time_exp() const;

  /**
   * Set the CPU time exponent.
   * @param e the new CPU time exponent.
   * @see time()
   */
  void time_exp( Mword e );

  /**
   * Get the CPU time mantissa.
   * @return The mantissa of the CPU time.
   * @see time()
   */
  Mword time_man() const;

  /**
   * Set the mantissa of the CPU time.
   * @param m the new mantissa of the CPU time.
   * @see time()
   */
  void time_man( Mword m );

  /**
   * Get the raw representation of the params.
   * @return The binary representation of the params.
   */
  Mword raw() const;

private:

  Mword _raw;

  enum {
    Prio_shift     = 0,
    Prio_mask      = 0x0ff,
    Prio_size      = 8,
    Small_shift    = Prio_size + Prio_shift,
    Small_size     = 8,
    Small_mask     = ((1UL << Small_size) - 1) << Small_shift,
    Mode_shift     = Small_shift + Small_size,
    Mode_size      = 4,
    Mode_mask      = ((1UL << Mode_size) - 1) << Mode_shift,
    Time_exp_shift = Mode_shift + Mode_size,
    Time_exp_size  = 4,
    Time_exp_mask  = ((1UL << Time_exp_size) - 1) << Time_exp_shift,
    Time_man_shift = Time_exp_size + Time_exp_shift,
    Time_man_size  = 8,
    Time_man_mask  = ((1UL << Time_man_size) - 1) << Time_man_shift,
  };

};

class L4_pipc
{
private:
  enum
    {
      Clock_shift        = 0,
      Id_shift           = 56,
      Lost_shift         = 62,
      Type_shift         = 63,
      Clock_mask         = 0x00ffffffffffffffULL,
      Id_mask            = 0x3f00000000000000ULL,
      Lost_mask          = 0x4000000000000000ULL,
      Type_mask          = 0x8000000000000000ULL,
      Low_shift          = 0,
      High_shift         = 32,
      Low_mask           = 0x00000000ffffffffULL,
      High_mask          = 0xffffffff00000000ULL
    };

  /**
   * Raw 64 bit representation
   */
  Unsigned64 _raw;

public:
  /**
   * Extract the low message word.
   */
  Mword low() const;

  /**
   * Extract the high message word.
   */
  Mword high() const;

  /**
   * Create a Preemption-IPC message
   */
  L4_pipc (unsigned type, unsigned lost, unsigned id, Cpu_time clock);
};


class L4_exception_ipc
{
public:
  enum
  {
    //Exception_ipc_cookie_1 = Mword(-0x5),
    //Exception_ipc_cookie_2 = Mword(-0x21504151),
    Protocol = -5
  };
};

class L4_quota_desc
{
public:
  enum Command
  { Nop = 0, Share = 1, New = 2 };

  explicit L4_quota_desc(Mword raw) : _d(raw) {}

  Command command() const;
  Task_num id() const;
  Mword amount() const;

private:
  Mword _d;
};



class L4_semaphore
{
public:
  Smword counter;
  Mword flags;
};


//----------------------------------------------------------------------------
INTERFACE [ia32 || ux]:

EXTENSION class L4_exception_ipc
{
public:
  enum { Msg_size = 16 };
};


//----------------------------------------------------------------------------
INTERFACE [arm]:

EXTENSION class L4_exception_ipc
{
public:
  enum { Msg_size = 20 };
};


//----------------------------------------------------------------------------
INTERFACE [amd64]:

EXTENSION class L4_exception_ipc
{
public:
  enum { Msg_size = 23 };
};


//----------------------------------------------------------------------------
INTERFACE [v2 || x0]:

EXTENSION class L4_uid
{
private:
  enum
    {
      Irq_mask           = 0x000000ffUL,
      Low_mask           = 0xffffffffUL,
      Nil_mask           = Low_mask,
      Version_low_mask   = 0x000003ffUL,
      Version_low_shift  = 0,
      Version_low_size   = 10,
      Lthread_mask       = 0x0001fc00UL,
      Lthread_shift      = Version_low_shift + Version_low_size,
      Lthread_size       = 7,
      Task_mask          = 0x0ffe0000UL,
      Task_shift         = Lthread_shift + Lthread_size,
      Task_size          = 11,
      Version_high_mask  = 0xf0000000UL,
      Version_high_shift = Task_shift + Task_size,
      Version_high_size  = 4,
    };

  Unsigned32 _raw;

public:
  /// must be constant since we build the spaces array from it
  enum
    {
      Max_tasks          = 1 << Task_size,
      Max_threads_per_task = 1 << Lthread_size,
    };

  /**
   * Extract the raw 64Bit representation.
   */
  Unsigned32 raw() const;

  /**
   * Create an L4-V2 UID from the raw 64Bit representation.
   */
  L4_uid (Unsigned32);

  /**
   * Create an L4-V2 UID.
   */
  L4_uid (Task_num task, LThread_num lthread, unsigned version = 0);
};


//----------------------------------------------------------------------------
IMPLEMENTATION:

PUBLIC inline
L4_msg_tag::L4_msg_tag(unsigned words, unsigned items, unsigned long flags,
    unsigned long proto)
  : _tag((words & 0x3f) | ((items << 6) & 0x3f) | flags | (proto << 16))
{}

PUBLIC inline
L4_msg_tag::L4_msg_tag(Mword raw)
  : _tag(raw)
{}


PUBLIC inline
unsigned long 
L4_msg_tag::raw() const
{ return _tag; }

PUBLIC inline
unsigned L4_msg_tag::words() const
{ return _tag & 63; }

PUBLIC inline
bool L4_msg_tag::transfer_fpu() const
{ return _tag & Transfer_fpu; }

//
// L4_timeout implementation
//

IMPLEMENT inline L4_timeout::L4_timeout (unsigned short t)
  : _t(t)
{}

IMPLEMENT inline unsigned short L4_timeout::raw() const
{ return _t; }

PUBLIC inline
Mword L4_timeout::abs_exp() const
{ return (_t >> 11) & 0xf; }

PUBLIC inline
bool L4_timeout::abs_clock() const
{ return _t & Clock_mask; }

IMPLEMENT inline
Unsigned64
L4_timeout::microsecs_rel (Unsigned64 clock) const
{
  if (man() == 0)
    return 0;
  else
   return clock + ((Unsigned64)man() << exp()); 
}

IMPLEMENT inline
Unsigned64
L4_timeout::microsecs_abs (Unsigned64 clock) const
{
  Mword e = abs_exp();
  Unsigned64 timeout = clock & ~((1 << e + 10) - 1) | man() << e;

  if (((clock >> e + 10) & 1) != abs_clock())
    timeout += 1 << e + 10;

  if (timeout < clock)
    return 0;

  return timeout;
}

PUBLIC inline
bool
L4_timeout::is_absolute() const
{ return _t & Abs_mask; }

PUBLIC inline
Unsigned64
L4_timeout::microsecs (Unsigned64 clock) const
{ 
  if (is_absolute())
    return microsecs_abs(clock);
  else
    return microsecs_rel(clock);
}

PUBLIC inline
bool L4_timeout::is_never() const
{ return !_t; }

PUBLIC inline
bool L4_timeout::is_zero() const
{ return _t == 0x0400; }

PUBLIC inline
unsigned short L4_timeout::is_finite() const
{ return _t; }


//
// L4_sched_param implementation
//

IMPLEMENT inline
L4_sched_param::L4_sched_param( Mword raw )
  : _raw(raw)
{}

IMPLEMENT inline Mword L4_sched_param::prio() const
{ return (_raw & Prio_mask) >> Prio_shift; }

IMPLEMENT inline void L4_sched_param::prio( Mword p )
{ _raw = (_raw & ~Prio_mask) | ((p << Prio_shift) & Prio_mask); }

IMPLEMENT inline Mword L4_sched_param::small() const
{ return (_raw & Small_mask) >> Small_shift; }

IMPLEMENT inline void L4_sched_param::small( Mword s )
{ _raw = (_raw & ~Small_mask) | ((s << Small_shift) & Small_mask); }

IMPLEMENT inline Mword L4_sched_param::time_exp() const
{ return (_raw & Time_exp_mask) >> Time_exp_shift; }

IMPLEMENT inline void L4_sched_param::time_exp( Mword e )
{ _raw = (_raw & ~Time_exp_mask) | ((e << Time_exp_shift) & Time_exp_mask); }

IMPLEMENT inline Mword L4_sched_param::time_man() const
{ return (_raw & Time_man_mask) >> Time_man_shift; }

IMPLEMENT inline void L4_sched_param::time_man( Mword m )
{ _raw = (_raw & ~Time_man_mask) | ((m << Time_man_shift) & Time_man_mask); }

IMPLEMENT inline Mword L4_sched_param::mode() const
{ return (_raw & Mode_mask) >> Mode_shift; }

IMPLEMENT inline void L4_sched_param::thread_state( Mword e )
{ _raw = (_raw & ~Mode_mask) | ((e << Mode_shift) & Mode_mask); }

IMPLEMENT inline Mword L4_sched_param::raw() const
{ return _raw; }

IMPLEMENT inline Mword L4_sched_param::is_valid() const
{ return _raw != (Mword)-1; }

IMPLEMENT inline
void
L4_sched_param::time (Unsigned64 t)
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
Unsigned64
L4_sched_param::time()
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

//
// L4_pipc implementation
//

IMPLEMENT inline
L4_pipc::L4_pipc (unsigned type, unsigned lost, unsigned id, Cpu_time clock)
       : _raw ( (((Unsigned64) type  << Type_shift)  & Type_mask) |
                (((Unsigned64) lost  << Lost_shift)  & Lost_mask) |
                (((Unsigned64) id    << Id_shift)    & Id_mask)   |
                (((Unsigned64) clock << Clock_shift) & Clock_mask))
{}

IMPLEMENT inline Mword L4_pipc::low() const
{ return (_raw & Low_mask) >> Low_shift; }

IMPLEMENT inline Mword L4_pipc::high() const
{ return (_raw & High_mask) >> High_shift; }


//
// Ipc_err implementation
//

IMPLEMENT inline
Ipc_err::Ipc_err (Mword raw)
       : _raw (raw & Cc_mask)
{}

IMPLEMENT inline Mword Ipc_err::raw() const
{ return _raw; }

IMPLEMENT inline Mword Ipc_err::deceited() const
{ return _raw & (1 << Deceite_bit); }

IMPLEMENT inline Mword Ipc_err::fpage_received() const
{ return _raw & (1 << Fpage_bit); }

IMPLEMENT inline Mword Ipc_err::redirected() const
{ return _raw & (1 << Redirected_bit); }

IMPLEMENT inline Mword Ipc_err::src_inside() const
{ return _raw & (1 << Src_inside_bit); }

IMPLEMENT inline Mword
Ipc_err::snd_error() const
{ return _raw & (1 << Snd_error_bit); }

IMPLEMENT inline Mword Ipc_err::has_error() const
{ return _raw & Error_mask; }

IMPLEMENT inline Mword Ipc_err::error() const
{ return _raw & Error_mask; }

// This function must not be inlined because in that case the str[] array
// would be inserted as often as an Ipc_err type is used anywhere. It
// is only an issue with gcc 2.95, newer gcc version handle this correctly.
IMPLEMENT
char const *
Ipc_err::str_error() const
{
  static char const * const str[] =
    {
      "OK",
      "Enot_existent", "Retimeout", "Setimeout", "Recanceled", "Secanceled",
      "Remapfailed", "Semapfailed", "Resndpfto", "Sesndpfto",  "Rercvpfto",
      "Sercvpfto", "Reaborted", "Seaborted", "Remsgcut", "Semsgcut"
    };

  return str [error() >> 4];
}

IMPLEMENT inline Mword Ipc_err::rcv_map_failed() const
{ return (_raw & Error_mask) == Remapfailed; }

IMPLEMENT inline Mword Ipc_err::snd_map_failed() const
{ return (_raw & Error_mask) == Semapfailed; }

IMPLEMENT inline void Ipc_err::error (Mword e)
{ _raw = (_raw & ~Error_mask) | (e & Error_mask); }

IMPLEMENT inline
void
Ipc_err::fpage_received (Mword w)
{
  if (w)
    _raw |= (1L << Fpage_bit);
  else
    _raw &= ~(1L << Fpage_bit);
}

IMPLEMENT inline void Ipc_err::combine (Ipc_err o)
{ _raw = (_raw & ~Error_mask) | o._raw; }


//
// L4_timeout implementation
//

IMPLEMENT inline
L4_timeout::L4_timeout (Mword man, Mword exp)
          : _t (((man & Man_mask) |
                ((exp << Exp_shift) & Exp_mask)))
{}

IMPLEMENT inline
L4_timeout::L4_timeout (Mword man, Mword exp, bool clock)
          : _t (((man & Man_mask) |
                ((exp << (Exp_shift+1)) & Exp_mask) |
		(clock ? Clock_mask : 0) | Abs_mask))
{}

IMPLEMENT inline Mword L4_timeout::exp() const
{ return (_t & Exp_mask) >> Exp_shift; }

IMPLEMENT inline void L4_timeout::exp (Mword w)
{ _t = (_t & ~Exp_mask) | ((w << Exp_shift) & Exp_mask); }

IMPLEMENT inline Mword L4_timeout::man() const
{ return (_t & Man_mask) >> Man_shift; }

IMPLEMENT inline void L4_timeout::man (Mword w)
{ _t = (_t & ~Man_mask) | ((w << Man_shift) & Man_mask); }

//
// L4_uid implementation
//

IMPLEMENT inline L4_uid::L4_uid()
{}

IMPLEMENT inline unsigned L4_uid::threads_per_task()
{ return 1 << Lthread_size; }

IMPLEMENT inline bool L4_uid::operator == (L4_uid const &o) const
{ return o._raw == _raw; }

IMPLEMENT inline Mword L4_uid::max_threads()
{ return 1 << (Task_size + Lthread_size); }

IMPLEMENT inline Mword L4_uid::irq() const
{ return _raw - 1; }

IMPLEMENT inline L4_uid L4_uid::irq (unsigned irq)
{ return L4_uid ((irq + 1) & Irq_mask); }

IMPLEMENT inline Mword L4_uid::is_irq() const
{ return (_raw & ~Irq_mask) == 0 && _raw; }

IMPLEMENT inline L4_uid L4_uid::task_id() const
{ return L4_uid (_raw & ~Lthread_mask); }

IMPLEMENT inline Mword L4_uid::is_nil() const
{ return _raw == 0; }

IMPLEMENT inline Mword L4_uid::is_invalid() const
{ return _raw == Invalid; }

IMPLEMENT inline bool L4_uid::is_valid() const
{ return !is_invalid(); }


IMPLEMENT inline
L4_quota_desc::Command 
L4_quota_desc::command() const
{ return (Command)(_d >> (MWORD_BITS-4)); }

IMPLEMENT inline
Task_num
L4_quota_desc::id() const
{ return _d  & 0xfff; }

IMPLEMENT inline
Mword
L4_quota_desc::amount() const
{ return _d & ~((0xfUL << (MWORD_BITS-4)) | 0xfff); }

IMPLEMENT inline
L4_uid::L4_uid (Unsigned32 w)
      : _raw (w)
{}

IMPLEMENT inline
L4_uid::L4_uid (Task_num task, LThread_num lthread, unsigned version)
  : _raw ((((Unsigned32) task    << Task_shift)         & Task_mask)        |
	  (((Unsigned32) lthread << Lthread_shift)      & Lthread_mask)     |
	  (((Unsigned32) version << Version_low_shift)  & Version_low_mask) |
	  (((Unsigned32) version << Version_high_shift) & Version_high_mask))
{}

IMPLEMENT inline Unsigned32 L4_uid::raw() const
{ return _raw; }

IMPLEMENT inline
unsigned
L4_uid::version() const
{
  return ((_raw & Version_high_mask) >> Version_high_shift) |
         ((_raw & Version_low_mask ) >> Version_low_shift);
}

IMPLEMENT inline
void
L4_uid::version (unsigned w)
{
  _raw = (_raw & ~(Version_low_mask | Version_high_mask)) |
         (((Unsigned32) w << Version_low_shift)  & Version_low_mask) |
         (((Unsigned32) w << Version_high_shift) & Version_high_mask);
}

IMPLEMENT inline
LThread_num
L4_uid::lthread() const
{
  // both casts (_raw and Lthread_mask) to unsigned are hints for gcc
  return ((unsigned) _raw & (unsigned) Lthread_mask) >> Lthread_shift;
}

IMPLEMENT inline
void
L4_uid::lthread (LThread_num w)
{
  _raw = (_raw & ~Lthread_mask) |
         (((Unsigned32) w << Lthread_shift) & Lthread_mask);
}

IMPLEMENT inline
Task_num
L4_uid::task() const
{
  // both casts (_raw and Task_mask) to unsigned are hints for gcc
  return ((unsigned) _raw & (unsigned) Task_mask) >> Task_shift;
}

IMPLEMENT inline
void
L4_uid::task (Task_num w)
{
  _raw = (_raw & ~Task_mask) |
         (((Unsigned32) w << Task_shift) & Task_mask);
}

IMPLEMENT inline
GThread_num
L4_uid::gthread() const
{
  // both casts (_raw and {LTHREAD,TASK}_MASK) to unsigned are hints for gcc
  return (((unsigned) _raw & (unsigned) Lthread_mask) >> Lthread_shift) |
         (((unsigned) _raw & (unsigned) Task_mask   ) >> (Task_shift -
							  Lthread_size));
}



IMPLEMENT inline
L4_uid::L4_uid (void *addr, Address tcb_base, Address tcb_size)
      : _raw ( (((unsigned long)addr - tcb_base) / tcb_size) << Lthread_shift )
{}

//
// L4_snd_desc implementation
//

IMPLEMENT inline L4_snd_desc::L4_snd_desc( Mword w )
  : _d(w)
{}

IMPLEMENT inline Mword L4_snd_desc::deceite() const
{ return _d & 1; }

IMPLEMENT inline Mword L4_snd_desc::map() const
{ return _d & 2; }

IMPLEMENT inline Mword L4_snd_desc::is_long_ipc() const
{ return _d & ~1; }

IMPLEMENT inline Mword L4_snd_desc::is_register_ipc() const
{ return (_d & ~1) == 0; }

IMPLEMENT inline Mword L4_snd_desc::has_snd() const
{ return _d != (Mword)-1; }

IMPLEMENT inline void *L4_snd_desc::msg() const
{ return (void*)(_d & ~3); }

PUBLIC inline Mword L4_snd_desc::raw() const
{ return _d; }

//
// L4_fpage implementation
//
IMPLEMENT inline
Mword
L4_fpage::is_valid() const
{ return _raw; }

PUBLIC static
inline
L4_fpage 
L4_fpage::all_spaces(unsigned long size = Whole_space)
{
  return L4_fpage(All_spaces_id | ((size << Size_shift) & Size_mask));
}

IMPLEMENT inline
bool
L4_fpage::is_all_spaces() const
{ return (_raw & Special_fp_mask) == All_spaces_id; }

IMPLEMENT inline
L4_fpage::L4_fpage(Raw raw)
  : _raw(raw)
{}

IMPLEMENT inline
L4_fpage::L4_fpage(Mword grant, Mword write, Mword size, Mword page,
    short cache)
  : _raw((grant ? (1<<Grant_bit) : 0)
	 | (write ? (1<<Write_bit) : 0)
	 | ((size << Size_shift) & Size_mask)
	 | ((page << Page_shift) & Page_mask)
	 | cache)
{}

IMPLEMENT inline
L4_fpage::L4_fpage(Mword size, Mword page)
  : _raw(((size << Size_shift) & Size_mask)
	 | ((page << Page_shift)
	    & Page_mask))
{}

IMPLEMENT inline
Mword
L4_fpage::grant() const
{ return _raw & (1<<Grant_bit); }

IMPLEMENT inline
Mword
L4_fpage::write() const
{ return _raw & (1<<Write_bit); }

IMPLEMENT inline
Mword
L4_fpage::size() const
{ return (_raw & Size_mask) >> Size_shift; }

IMPLEMENT inline
Mword
L4_fpage::page() const
{ return (_raw & Page_mask) >> Page_shift; }

IMPLEMENT inline
void
L4_fpage::grant(Mword w)
{
  if(w)
    _raw |= (1<<Grant_bit);
  else
    _raw &= ~(1<<Grant_bit);
}

IMPLEMENT inline
void
L4_fpage::write(Mword w)
{
  if(w)
    _raw |= (1<<Write_bit);
  else
    _raw &= ~(1<<Write_bit);
}

IMPLEMENT inline
void
L4_fpage::size(Mword w)
{ _raw = (_raw & ~Size_mask) | ((w<<Size_shift) & Size_mask); }

IMPLEMENT inline
void
L4_fpage::page(Mword w)
{ _raw = (_raw & ~Page_mask) | ((w<<Page_shift) & Page_mask); }

IMPLEMENT inline
void 
L4_fpage::status (Mword status)
{ _raw = (_raw & ~Status_mask) | status; }

IMPLEMENT inline
Mword 
L4_fpage::status() const
{ return _raw & Status_mask; }

IMPLEMENT inline
L4_fpage::Raw
L4_fpage::raw() const
{ return _raw; }

IMPLEMENT inline
Mword
L4_fpage::is_whole_space() const
{ return (_raw >> 2) == Whole_space; }

PUBLIC inline
L4_fpage::Cache_type
L4_fpage::cache_type() const
{ return _raw & Cache_type_mask; }

//
// L4_rcv_desc implementation
//

IMPLEMENT inline L4_rcv_desc L4_rcv_desc::short_fpage (L4_fpage fp)
{ return L4_rcv_desc( 2 /*rmap*/ | (fp.raw() & ~3) ); }

IMPLEMENT inline L4_rcv_desc::L4_rcv_desc( Mword w = (Mword)-1 )
  : _d(w)
{}

IMPLEMENT inline Mword L4_rcv_desc::open_wait() const
{ return _d & 1; }

IMPLEMENT inline Mword L4_rcv_desc::rmap() const
{ return _d & 2; }

IMPLEMENT inline void *L4_rcv_desc::msg() const
{ return (void*)(_d & ~3); }

IMPLEMENT inline L4_fpage L4_rcv_desc::fpage() const
{ return L4_fpage( _d & ~3 ); }

IMPLEMENT inline Mword L4_rcv_desc::is_register_ipc() const
{ return (_d & ~1) == 0; }

IMPLEMENT inline Mword L4_rcv_desc::has_receive() const
{ return _d != (Mword)-1; }

/// for debugging (jdb)
PUBLIC inline Mword L4_rcv_desc::raw() const
{ return _d; }

//
// L4_msgdope implementation
//

IMPLEMENT inline
L4_msgdope::L4_msgdope (L4_snd_desc send_desc, Mword mwords, Mword strings)
  : _raw( ((strings << Strings_shift) & Strings_mask)
	  |((mwords << Mwords_shift) & Mwords_mask)
	  |(send_desc.raw() & (1<<Fpage_bit)))
{}

IMPLEMENT inline
L4_msgdope::L4_msgdope( Mword raw )
  : _raw(raw)
{}

IMPLEMENT inline
L4_msgdope::L4_msgdope (Ipc_err e)
  : _raw (e.raw())
{}

IMPLEMENT inline Mword L4_msgdope::raw() const
{ return _raw; }

IMPLEMENT inline Mword L4_msgdope::raw_dope() const
{return _raw & Dope_mask;}

IMPLEMENT inline Mword L4_msgdope::deceited() const
{ return _raw & (1 << Deceite_bit); }

IMPLEMENT inline Mword L4_msgdope::fpage_received() const
{ return _raw & (1 << Fpage_bit); }

IMPLEMENT inline Mword L4_msgdope::redirected() const
{ return _raw & (1 << Redirected_bit); }

IMPLEMENT inline Mword L4_msgdope::src_inside() const
{ return _raw & (1 << Src_inside_bit); }

IMPLEMENT inline Mword L4_msgdope::snd_error() const
{ return _raw & (1 << Snd_error_bit); }

IMPLEMENT inline Mword L4_msgdope::strings() const
{ return (_raw & Strings_mask) >> Strings_shift; }

IMPLEMENT inline Mword L4_msgdope::mwords() const
{ return (_raw & Mwords_mask) >> Mwords_shift; }

IMPLEMENT inline void L4_msgdope::mwords( Mword w )
{ _raw = (_raw & ~Mwords_mask) | ((w << Mwords_shift) & Mwords_mask); }

IMPLEMENT inline void L4_msgdope::strings( Mword w )
{ _raw = (_raw & ~Strings_mask) | ((w << Strings_shift) & Strings_mask); }

IMPLEMENT inline Mword L4_msgdope::has_error() const
{ return _raw & Error_mask; }

IMPLEMENT inline Mword L4_msgdope::error() const
{ return _raw & Error_mask; }

// This function must not be inlined because in that case the str[] array
// would be inserted as often as an L4_msgdope type is used anywhere. It
// is only an issue with gcc 2.95, newer gcc version handle this correctly.
IMPLEMENT
char const *
L4_msgdope::str_error() const
{
  static char const * const str[] =
    {
      "OK",
      "Enot_existent", "Retimeout", "Setimeout", "Recanceled", "Secanceled",
      "Remapfailed", "Semapfailed", "Resndpfto", "Sesndpfto",  "Rercvpfto",
      "Sercvpfto", "Reaborted", "Seaborted", "Remsgcut", "Semsgcut"
    };

  return str[error() >> 4];
}

IMPLEMENT inline Mword L4_msgdope::rcv_map_failed() const
{ return (_raw & Error_mask) == Remapfailed; }

IMPLEMENT inline Mword L4_msgdope::snd_map_failed() const
{ return (_raw & Error_mask) == Semapfailed; }

IMPLEMENT inline void L4_msgdope::error (Mword e)
{ _raw = (_raw & ~Error_mask) | (e & Error_mask); }

IMPLEMENT inline
void
L4_msgdope::fpage_received( Mword w )
{
  if(w)
    _raw |= (1L << Fpage_bit);
  else
    _raw &= ~(1L << Fpage_bit);
}

IMPLEMENT inline void L4_msgdope::combine( L4_msgdope o )
{ _raw = (_raw & ~Error_mask) | o._raw; }

IMPLEMENT inline void L4_msgdope::combine( Ipc_err e )
{ _raw = (_raw & ~Error_mask) | e.raw(); }

