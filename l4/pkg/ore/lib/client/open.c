#include <stdlib.h>
#include <l4/log/l4log.h>
#include "local.h"

l4_threadid_t ore_server = L4_INVALID_ID;

// perform open actions
l4ore_handle_t ore_do_open(const char *dev, unsigned char mac[6],
                           l4dm_dataspace_t *send, l4dm_dataspace_t *recv,
                           l4ore_config *conf)
{
  DICE_DECLARE_ENV(_dice_corba_env);
  _dice_corba_env.malloc            = (dice_malloc_func)malloc;
  _dice_corba_env.free              = (dice_free_func)free;
  l4ore_handle_t ret;
  l4ore_config _conf;

  l4dm_dataspace_t dummy            = L4DM_INVALID_DATASPACE;

  // evil user provides us with no space for the MAC...
  if (mac == NULL)
    {
      LOG("No memory allocated for MAC address return value.");
      return -L4_ENOMEM;
    }

  // gracefully handle NULL pointer for config
  if (conf == NULL)
    {
      LOG("No memory allocated for connection configuration.");
      _conf = L4ORE_DEFAULT_CONFIG;
    }
  else
    _conf = *conf;

  if (ore_lookup_server())
    return -1;

  // We cannot send NULL values through Dice-IPC, therefore we
  // set the dataspaces to invalid if necessary
  if (send == NULL)
    send = &dummy;
  if (recv == NULL)
    recv = &dummy;

  // open()
  ret = ore_ore_open_call(&ore_server, dev, mac, send, recv,
                          &_conf, &_dice_corba_env);

  // print error if necessary
  if (ret < 0)
    LOG_Error("ORe error: %s", l4env_strerror(-ret));

  if (conf)
    *conf = _conf;

  return ret;
}
