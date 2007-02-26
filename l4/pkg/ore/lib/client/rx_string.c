#include "local.h"
#include <stdlib.h>
#include <dice/dice.h>

int ore_recv_string_blocking(l4ore_handle_t channel, int handle,
                             char **data, l4_size_t *size, 
                             l4_timeout_t timeout)
{
  int ret;
  l4_size_t real_size = *size;  // is also used as receive size by Dice code
  DICE_DECLARE_ENV(_dice_corba_env);
  _dice_corba_env.malloc = (dice_malloc_func)malloc;
  _dice_corba_env.free   = (dice_free_func)free;

  _dice_corba_env.timeout = timeout;
//  LOG("receiving from "l4util_idfmt, l4util_idstr(channel));
  ret = ore_rxtx_recv_call(&channel, data, *size, &real_size,
                          ORE_BLOCKING_CALL, &_dice_corba_env);

  // check CORBA exceptions
  if (DICE_HAS_EXCEPTION(&_dice_corba_env))
    {
      switch (DICE_EXCEPTION_MINOR(&_dice_corba_env))
        {
        case CORBA_DICE_EXCEPTION_IPC_ERROR:
//          LOG("IPC error = %d", DICE_IPC_ERROR(&_dice_corba_env));
//          LOG("Target = "l4util_idfmt, l4util_idstr(channel));
          switch (DICE_IPC_ERROR(&_dice_corba_env))
            {
              /* REMSGCUT means that our rx buffer was too small.
               * real_size will then contain the buffer size we need to
               * receive the packet.
               */
            case L4_IPC_REMSGCUT:       // buffer too small
              ret = real_size;
              break;
            case L4_IPC_ENOT_EXISTENT:  // no partner !?
              ret = -L4_ENOTFOUND;
              break;
            case L4_IPC_SETIMEOUT:
              // fall through
            case L4_IPC_RETIMEOUT:      // timeout expired
              ret = -L4_ETIME;
              break;
            default:
              ret = -L4_EIPC;
              break;
            }
          break;
        default:
          // unknown error - should not happen.
          LOG_Error("Unknown CORBA error: %d", 
	      DICE_EXCEPTION_MINOR(&_dice_corba_env));
          ret = -L4_EUNKNOWN;
          break;
        }
    }
  else
    *size = real_size;

  return ret;
}

int ore_recv_string_nonblocking(l4ore_handle_t channel, int handle, 
                                char **data, unsigned int *size)
{
  unsigned int ret;
  unsigned int real_size;
  DICE_DECLARE_ENV(_dice_corba_env);
  _dice_corba_env.malloc = (dice_malloc_func)malloc;
  _dice_corba_env.free   = (dice_free_func)free;

  ret = ore_rxtx_recv_call(&channel, data, *size, &real_size,
                          ORE_NONBLOCKING_CALL, &_dice_corba_env);

  // check CORBA exceptions
  if (DICE_HAS_EXCEPTION(&_dice_corba_env))
    {
      switch (DICE_EXCEPTION_MINOR(&_dice_corba_env))
        {
        case CORBA_DICE_EXCEPTION_IPC_ERROR:
//          LOG("IPC error = %d", DICE_IPC_ERROR(&_dice_corba_env));
          switch (DICE_IPC_ERROR(&_dice_corba_env))
            {
              /* REMSGCUT means that our rx buffer was too small.
               * real_size will then contain the buffer size we need to
               * receive the packet.
               */
            case L4_IPC_REMSGCUT:
              ret = real_size;
              break;
            case L4_IPC_ENOT_EXISTENT:
              ret = -L4_ENOTFOUND;
              break;
            default:
              ret = -L4_EIPC;
              break;
            }
          break;
        default:
          // unknown error - should not happen.
          ret = -L4_EUNKNOWN;
          break;
        }
    }
  else
    *size = real_size;

  return ret;
}
