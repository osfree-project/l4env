#include "ore-local.h"

/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden				*
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

void dump_connection(int);

int ore_debug = ORE_DEBUG_DEFFAULT;

/* Component function for the ORe configure() call. Adapts the
 * channel configuration and returns the old config settings
 * through the old_conf parameter.
 *
 * 1. May be called with an invalid ORe handle. In this case
 *    we only read the debug configuration from the config
 *    descriptor and set it accordingly. This enables clients
 *    to setup debugging even if they do not already have an
 *    open connection to ORe.
 *
 * 2. If the ORe handle is valid, we return the current channel
 *    configuration and set the rw properties as given in the 
 *    new configuration.
 */
void ore_manager_configure_component(CORBA_Object _dice_corba_obj,
                                 const l4ore_handle_t *ch,
                                 const l4ore_config *new_conf,
                                 l4ore_config *old_conf,
                                 CORBA_Server_Environment *_dice_corba_env)
{
  int channel = find_channel_for_worker(*ch);

  // debug flag is global - we even set it, if the channel is incorrect
  // (this is a feature!)
  ore_debug   = new_conf->rw_debug;

  LOGd(ORE_DEBUG, "Debug level: %d\n", ore_debug);

  if (channel < 0 || channel > ORE_CONFIG_MAX_CONNECTIONS)
    return;

  *old_conf   = ore_connection_table[channel].config;
  if (!l4ore_is_invalid_config(*new_conf))
    {
      ore_connection_table[channel].config.rw_debug       = new_conf->rw_debug;
      ore_connection_table[channel].config.rw_broadcast   = new_conf->rw_broadcast;
      ore_connection_table[channel].config.rw_active      = new_conf->rw_active;
    }
}

void dump_connection(int i)
{
    ore_connection_t *con = &ore_connection_table[i];    
    LOG("\033[36mdumping connection #%d\033[0m", i);
    LOG("Owner      = "l4util_idfmt"   Device    = %s", l4util_idstr(con->owner), con->dev->name);
    LOG("Flags      = %x", con->flags);
    LOG_MAC(1, &con->mac[0]);
    LOG("Packets inc %d, deliv %d, queued %d, tx %d", con->packets_received,
		con->packets_received - con->packets_queued, con->packets_queued,
		con->packets_sent);
    LOG("- - - - - - - - - - - - - - - - - - - - - -");
}

#ifdef CONFIG_ORE_DUMPER
void dump_periodic(void *argp)
{
    int i;
    
    LOG("The periodic dumper has been started.");

    while(1)
    {
        l4_sleep(CONFIG_ORE_DUMPER_PERIOD * 1000);
        LOG("=============================================");
        LOG("dumping connection table.");
        for (i=0; i<ORE_CONFIG_MAX_CONNECTIONS; i++)
        {
            if (ore_connection_table[i].in_use)
                dump_connection(i);
        }
        LOG("=============================================");
    }
}
#endif // dumper
