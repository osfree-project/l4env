INTERFACE:

extern "C" void _only_1_2_or_4_byte_values_can_be_handled_by_peek_poke_user();

/** Various helper functions for thread that differ for
 *  the implementation with small address spaces.
 */
IMPLEMENTATION[ia32-smas]:

#include "smas.h"
#include "l4_types.h"


PRIVATE static inline 
void Thread::memcpy_byte_fs_es(void *dst, void const *src, size_t n)
{
  unsigned dummy1, dummy2, dummy3;
  asm volatile ( " cld                                     \n"
		 " repz movsl %%fs:(%%esi), %%es:(%%edi)   \n"
		 " mov %%edx, %%ecx                        \n"
		 " repz movsb %%fs:(%%esi), %%es:(%%edi)   \n"
		 :
		 "=c"(dummy1), "=S"(dummy2), "=D"(dummy3)
		 :
		 "c"(n>>2), "d"(n & 3), "S"(src), "D"(dst)
		 : 
		 "memory");
}

PRIVATE static inline
void Thread::memcpy_mword_fs_es(void *dst, void const *src, size_t n)
{
  unsigned dummy1, dummy2, dummy3;
  asm volatile ( " cld                                     \n"
		 " repz movsl %%fs:(%%esi), %%es:(%%edi)   \n"
		 :
		 "=c"(dummy1), "=S"(dummy2), "=D"(dummy3)
		 :
		 "c"(n), "S"(src), "D"(dst)
		 :
		 "memory");
}


/**
 * Additional things to do before killing, when using small spaces.
 * Switch to idle thread in case the killer thread runs in a
 * small address space while the one to be killed happens to be
 * the current big one. If we just deleted it the small killer 
 * would be floating in the great big nothing.
 */
IMPLEMENT inline NEEDS ["l4_types.h"]
void Thread::kill_small_space (void)
{
      if ( space() == current_space() ) {
        current()->switch_to(lookup_thread(L4_uid(L4_uid::NIL)));
      }

      smas.move( space(), 0);

}


/**
 * Return small address space the task is in.
 */
IMPLEMENT inline NEEDS["smas.h"]
Mword Thread::small_space( void )
{
  return smas.space_nr( space() );   // unimplemented
}

/**
 * Move the task this thread belongs to to the given small address space
 */
IMPLEMENT inline NEEDS["smas.h"]
void Thread::set_small_space( Mword nr)
{
    if ( nr < 255 ) {
        if (nr == 0) {
          /* XXX Seems like everybody assumes that 0  here means: do nothing.
           * Therefore moving a task out of a small space this way
           * (what it should be doing according to the manual!) is 
           * disabled by now.
           */
//          smas.move( space(), 0 );
        } else {
          int spacesize = 1;
          while ( !(nr & 1) ) {
            spacesize <<= 1;
            nr >>= 1;
          }
          smas.set_space_size( spacesize );
          smas.move( space(), nr >> 1 );
       }
      }
  
}


IMPLEMENT inline NEEDS ["smas.h"]
bool
Thread::handle_smas_page_fault( Address pfa, Mword error_code,
				     L4_msgdope &ipc_code)
{
  Address smaddr;          // for finding out about small spaces
  Space_context *smspace;      // dito

  if ( smas.linear_to_small( pfa, &smspace, &smaddr ) )
  {
    // doesn't work just yet
    if (space_index() == Config::sigma0_taskno) 
    {
      panic("Sigma0 cannot (yet) run in a small space.");
    }

    //only interested in ourselves
    if ( space() == smspace)
    {
      bool writable;

      //lazy updating...
      if (EXPECT_TRUE (smspace->lookup (smaddr, &writable) != (Address) -1 &&
	                      (writable || !(error_code & PF_ERR_WRITE))))

          {
            current_space()->remote_update (pfa, smspace, smaddr, 1);
            return true;
          }

      // Didn't work? Seems the pager is needed.
      if (!(ipc_code = handle_page_fault_pager(smaddr, error_code)).has_error())
        // now copy it in again
        // strange but right: may not be the same space as before
          current_space()->remote_update (pfa, smspace, smaddr, 1);
          return true;
    }
  }
  
  //give up
  return false;
}

IMPLEMENT inline NEEDS["smas.h","l4_types.h"]
bool Thread::handle_smas_gp_fault(Mword error_code)
{
  if (((error_code & 0xffff) == 0)
       && space()->is_small_space()) 
  {
    smas.move( space(), 0 );
    printf( "KERNEL: Space exceeded? Move task %u out of small space.\n",
                  unsigned(space_index()) );
    return true;
  }
  
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
IMPLEMENT inline NEEDS[Thread::memcpy_byte_fs_es]
template< typename T >
void Thread::copy_from_user(T *kdst, T const *usrc, size_t n)
{
  assert (this == current());

  // copy from user byte by byte
  memcpy_byte_fs_es( (void*)kdst, (void*)usrc, n*sizeof(T) );
}

IMPLEMENT inline NEEDS[Thread::memcpy_mword_fs_es]
template<>
void Thread::copy_from_user<Mword>(Mword *kdst, Mword const *usrc, size_t n)
{
  assert (this == current());

  // copy from user word by word
  memcpy_mword_fs_es( (void*)kdst, (void*)usrc, n );
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
IMPLEMENT inline NEEDS[Thread::memcpy_byte_ds_es]
template< typename T >
void Thread::copy_to_user(T *udst, T const *ksrc, size_t n)
{
  assert (this == current());

  // copy from user byte by byte
  asm ("mov %%fs, %%eax; mov %%eax, %%es" : : : "eax");
  memcpy_byte_ds_es( (void*)udst, (void*)ksrc, n * sizeof(T) );
  asm ("mov %%ds, %%eax; mov %%eax, %%es" : : : "eax");
}

IMPLEMENT inline NEEDS[Thread::memcpy_mword_ds_es]
template<>
void Thread::copy_to_user<Mword>(Mword *udst, Mword const *ksrc, size_t n)
{
  assert (this == current());

  // copy from user word by word
  asm ("mov %%fs, %%eax; mov %%eax, %%es" : : : "eax");
  memcpy_mword_ds_es( (void*)udst, (void*)ksrc, n );
  asm ("mov %%ds, %%eax; mov %%eax, %%es" : : : "eax");
}



IMPLEMENT inline
template < typename T >
void Thread::poke_user( T *addr, T value)
{
  assert (this == current());

  switch(sizeof(T)) 
    {
    case 1:
      asm ("movb %b0, %%fs:(%1)"
	   :
	   : "Nr"(value), "Nr"(addr)
	   : "memory"
	   );
      break;
    case 2:
      asm ("movw %w0, %%fs:(%1)"
	   :
	   : "Nr"(value), "Nr"(addr)
	   : "memory"
	   );
      break;
    case 4:
      asm ("movl %0, %%fs:(%1)"
	   :
	   : "Nr"(value), "Nr"(addr)
	   : "memory"
	   );
      break;
    default:
      copy_to_user<T>(addr,&value,1);
      //_only_1_2_or_4_byte_values_can_be_handled_by_peek_poke_user();
      break;
  
    }
}


IMPLEMENT inline
template< typename T >
T Thread::peek_user( T const *addr )
{
  assert (this == current());

  T ret;    
  switch(sizeof(T)) 
    {
    case 1:
      asm ("movb %%fs:(%1), %b0"
	   : "=r"(ret)
	   : "Nr"(addr)
	   );
      break;
    case 2:
      asm ("movw %%fs:(%1), %w0"
	   : "=r"(ret)
	   : "Nr"(addr)
	   );
      break;
    case 4:
      asm ("movl %%fs:(%1), %0"
	   : "=r"(ret)
	   : "Nr"(addr)
	   );
      break;
    default:
      copy_from_user<T>(&ret, addr, 1);
      //_only_1_2_or_4_byte_values_can_be_handled_by_peek_poke_user();
      break;

    }
  return ret;
}


//-----------------------------------------------------------------------------

