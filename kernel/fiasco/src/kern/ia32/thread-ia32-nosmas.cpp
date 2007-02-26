IMPLEMENTATION[ia32-nosmas]:

IMPLEMENT inline
bool Thread::handle_smas_gp_fault(Mword)
{
  return false;
}


//-----------------------------------------------------------------------------
// User space access functions
//-----------------------------------------------------------------------------

/**
 * Copy n Mwords from virtual user address usrc to virtual kernel address kdst.
 * Normally this is done with GCC's memcpy. When using small address spaces,
 * though, we us the GS segment for access to user space, so we don't mind
 * being moved around address spaces while copying.
 * @brief Copy between user and kernel address space
 * @param kdst Destination address in kernel space
 * @param usrc Source address in user space
 * @param n Number of Mwords to copy
 */
IMPLEMENT inline NEEDS[Thread::memcpy_byte_ds_es]
template< typename T >
void Thread::copy_from_user(void *kdst, void const *usrc, size_t n)
{
  assert (this == current());

  // copy from user byte by byte
  memcpy_byte_ds_es( kdst, usrc, n*sizeof(T) );
}

IMPLEMENT inline NEEDS [Thread::memcpy_mword_ds_es]
template<>
void Thread::copy_from_user<Mword>(void *kdst, void const *usrc, size_t n)
{
  assert (this == current());

  // copy from user word by word
  memcpy_mword_ds_es( kdst, usrc, n );
}


/**
 * Copy n Mwords from virtual user address usrc to virtual kernel address kdst.
 * Normally this is done with GCC's memcpy. When using small address spaces,
 * though, we us the GS segment for access to user space, so we don't mind
 * being moved around address spaces while copying.
 * @brief Copy between user and kernel address space
 * @param kdst Destination address in kernel space
 * @param usrc Source address in user space
 * @param n Number of Mwords to copy
 */
IMPLEMENT inline NEEDS [Thread::memcpy_byte_ds_es]
template< typename T >
void Thread::copy_to_user(void *udst, void const *ksrc, size_t n)
{
  assert (this == current());

  // copy from user byte by byte
  memcpy_byte_ds_es( udst, ksrc, n*sizeof(T) );
}

IMPLEMENT inline NEEDS [Thread::memcpy_mword_ds_es]
template<>
void Thread::copy_to_user<Mword>(void *udst, void const *ksrc, size_t n)
{
  assert (this == current());

  // copy from user word by word
  memcpy_mword_ds_es( udst, ksrc, n );
}



IMPLEMENT inline
template < typename T >
void Thread::poke_user( T *addr, T value)
{
  assert (this == current());

  *addr = value;
}


IMPLEMENT inline
template< typename T >
T Thread::peek_user( T const *addr )
{
  assert (this == current());

  return *addr;
}

//-----------------------------------------------------------------------------


