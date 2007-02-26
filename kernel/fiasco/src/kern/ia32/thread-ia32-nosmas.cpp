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
void Thread::copy_from_user(T *kdst, T const *usrc, size_t n)
{
  assert (this == current());

  // copy from user byte by byte
  memcpy_byte_ds_es( (void*)kdst, (void*)usrc, n*sizeof(T) );
}

IMPLEMENT inline NEEDS [Thread::memcpy_mword_ds_es]
template<>
void Thread::copy_from_user<Mword>(Mword *kdst, Mword const *usrc, size_t n)
{
  assert (this == current());

  // copy from user word by word
  memcpy_mword_ds_es( (void*)kdst, (void*)usrc, n );
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
void Thread::copy_to_user(T *udst, T const *ksrc, size_t n)
{
  assert (this == current());

  // copy from user byte by byte
  memcpy_byte_ds_es( (void*)udst, (void*)ksrc, n*sizeof(T) );
}

IMPLEMENT inline NEEDS [Thread::memcpy_mword_ds_es]
template<>
void Thread::copy_to_user<Mword>(Mword *udst, Mword const *ksrc, size_t n)
{
  assert (this == current());

  // copy from user word by word
  memcpy_mword_ds_es( (void*)udst, (void*)ksrc, n );
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


