INTERFACE:

EXTENSION class L4_fpage
{
public:
  /**
   * task cap specific constants.
   */
  enum {
    Whole_cap_space = 11, ///< The order used to cover the whole task-cap space
    Cap_max    = 1L << Whole_cap_space, ///< Number of available task caps.
  };

  /**
   * Create the given task flex page.
   * @param port the port address.
   * @param order the size of the flex page is 2^order.
   * @param grant if not zero the grant bit is to be set.
   */
  static L4_fpage task_cap(Mword taskno, Mword order, Mword grant);

  /**
   * Get the task-cap flexpage base address.
   * @return The task-cap flexpage base address.
   */
  Mword task() const;

  /**
   * Set the task-cap flexpage base address.
   * @param addr the task-cap flexpage base address.
   */
  void task( Mword taskno );

  /**
   * Is the flex page a task-cap flex page?
   * @returns not zero if this flex page is a task-cap flex page.
   */
  Mword is_cappage() const;

  /**
   * Does the flex page cover the whole task-cap space.
   * @pre The is_cappage() method must return true or the 
   *      behavior is undefined.
   * @return not zero if the flex page covers the whole task-cap
   *          space.
   */
  Mword is_whole_cap_space() const;

private:
  enum {
    Cap_mask  = 0x007ff000,
    Cap_shift = 12,
  };
};

INTERFACE [32bit]:
  
EXTENSION class L4_fpage
{
private:
  enum 
  {
    Cap_id        = 0xf0000100,
  };
};

INTERFACE [64bit]:
  
EXTENSION class L4_fpage
{
private:
  enum 
  {
    Cap_id        = 0xfffffffff0000100UL,
  };
};

//---------------------------------------------------------------------------
IMPLEMENTATION [!caps]:

IMPLEMENT inline
Mword L4_fpage::task() const
{
  return 0;
}

IMPLEMENT inline
Mword L4_fpage::is_cappage() const
{
  return 0;
}

//---------------------------------------------------------------------------
IMPLEMENTATION [caps]:

IMPLEMENT inline
Mword L4_fpage::is_cappage() const
{
  return (_raw & Special_fp_mask) == Cap_id;
}

IMPLEMENT inline
void L4_fpage::task( Mword w )
{
  _raw = (_raw & ~Cap_mask) | ((w << Cap_shift) & Cap_mask);
}

IMPLEMENT inline
Mword L4_fpage::task() const
{
  return (_raw & Cap_mask) >> Cap_shift;
}

IMPLEMENT inline
L4_fpage L4_fpage::task_cap( Mword taskno, Mword order, Mword grant)
{
  return L4_fpage( (grant ? 1 : 0)
		   | ((taskno << Cap_shift) & Cap_mask)
		   | ((order << Size_shift) & Size_mask)
		   | Cap_id);
}

IMPLEMENT inline
Mword L4_fpage::is_whole_cap_space() const
{
  return (_raw >> 2) == Whole_cap_space;
}
