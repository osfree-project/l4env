#include "local.h"
#include <l4/names/libnames.h>

// evil: call configure() with invalid channel number. this
//       will only change the debug state
void l4ore_debug(l4ore_config *c, int flag)
{
  DICE_DECLARE_ENV(env);
  l4ore_handle_t handle = L4_INVALID_ID;
  l4ore_config conf;
  l4_threadid_t server_id;

  env.malloc = (dice_malloc_func)malloc;
  env.free   = (dice_free_func)free;
  conf.rw_debug = flag;

  if (strlen(c->ro_orename) == 0)
      strcpy(conf.ro_orename, c->ro_orename);
  else
      strcpy(conf.ro_orename, "ORe");

  if (ore_lookup_server(c->ro_orename, &server_id))
  {
	  LOG("could not lookup server %s\n", c->ro_orename);
	  return;
  }

  ore_manager_configure_call(&server_id, &handle, &conf, &conf, &env);
}

l4ore_config l4ore_get_config(int channel)
{
  DICE_DECLARE_ENV(env);
  l4ore_config conf;
  l4ore_config inval = L4ORE_INVALID_CONFIG;
  l4ore_handle_t handle = descriptor_table[channel].remote_worker_thread;
  l4_threadid_t ore_server = descriptor_table[channel].remote_manager_thread;
  env.malloc = (dice_malloc_func)malloc;
  env.free   = (dice_free_func)free;

  if (l4_is_invalid_id(handle))
    return L4ORE_INVALID_CONFIG;
  
  ore_manager_configure_call(&ore_server, &handle, &inval, &conf, &env);

  return conf;
}

void l4ore_set_config(int channel, l4ore_config *new_conf)
{
  DICE_DECLARE_ENV(env);
  l4ore_config old;
  l4ore_handle_t handle = descriptor_table[channel].remote_worker_thread;
  l4_threadid_t ore_server = descriptor_table[channel].remote_manager_thread;
  env.malloc = (dice_malloc_func)malloc;
  env.free   = (dice_free_func)free;
 
  if (new_conf == NULL)
      return;

  ore_manager_configure_call(&ore_server, &handle, new_conf, &old, &env);
}

