#include "local.h"
#include <stdlib.h>
#include <dice/dice.h>

int ore_recv_string_blocking(l4ore_handle_t channel, char **data,
                             unsigned int *size)
{
  int ret;
  int real_size;
  CORBA_Environment _dice_corba_env = dice_default_environment;

  ret = ore_ore_recv_call(&ore_server, channel, data, size, &real_size,
                          ORE_BLOCKING_CALL, &_dice_corba_env);

  // check CORBA exceptions
  if (_dice_corba_env.major != CORBA_DICE_EXCEPTION_NONE)
    {
      LOG("IPC error = %d", _dice_corba_env._p.ipc_error);
      switch (_dice_corba_env.major)
        {
        case CORBA_DICE_EXCEPTION_IPC_ERROR:
          switch (_dice_corba_env._p.ipc_error)
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

int ore_recv_string_nonblocking(l4ore_handle_t channel,
                                char **data, unsigned int *size)
{
  int ret;
  int real_size;
  CORBA_Environment _dice_corba_env = dice_default_environment;

  ret = ore_ore_recv_call(&ore_server, channel, data, size, &real_size,
                          ORE_NONBLOCKING_CALL, &_dice_corba_env);

  // check CORBA exceptions
  if (_dice_corba_env.major != CORBA_DICE_EXCEPTION_NONE)
    {
      LOG("IPC error = %d", _dice_corba_env._p.ipc_error);
      switch (_dice_corba_env.major)
        {
        case CORBA_DICE_EXCEPTION_IPC_ERROR:
          switch (_dice_corba_env._p.ipc_error)
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
