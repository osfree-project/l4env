
#include "ore-local.h"

int ore_debug = ORE_DEBUG_DEFFAULT;

void ore_ore_configure_component(CORBA_Object _dice_corba_obj,
                                 l4ore_handle_t channel,
                                 const l4ore_config *new_conf,
                                 l4ore_config *old_conf,
                                 CORBA_Server_Environment *_dice_corba_env)
{
  // debug flag is global - we even set it, if the channel is incorrect
  // (this is a feature!)
  ore_debug   = new_conf->rw_debug;

  printf("Debug level: %d\n", ore_debug);

  if (channel < 0 || channel > ORE_CONFIG_MAX_CONNECTIONS)
    return;

  l4lock_lock(&ore_connection_table[channel].channel_lock);

  *old_conf   = ore_connection_table[channel].config;
  if (!l4ore_is_invalid_config(*new_conf))
    {
      ore_connection_table[channel].config.rw_debug       = new_conf->rw_debug;
      ore_connection_table[channel].config.rw_broadcast   = new_conf->rw_broadcast;
      ore_connection_table[channel].config.rw_active      = new_conf->rw_active;
    }

  l4lock_unlock(&ore_connection_table[channel].channel_lock);
}
