#include "local.h"

// evil: call configure() with invalid channel number. this
//       will only change the debug state
void l4ore_debug(int flag)
{
  DICE_DECLARE_ENV(env);
  l4ore_config conf;
  conf.rw_debug = flag;

  if (ore_lookup_server())
    return;

  ore_ore_configure_call(&ore_server, -1, &conf, &conf, &env);
}

l4ore_config l4ore_get_config(l4ore_handle_t channel)
{
  DICE_DECLARE_ENV(env);
  l4ore_config conf;
  l4ore_config inval = L4ORE_INVALID_CONFIG;

  if (l4_thread_equal(ore_server, L4_INVALID_ID))
    return L4ORE_INVALID_CONFIG;

  ore_ore_configure_call(&ore_server, channel, &inval, &conf, &env);

  return conf;
}

void l4ore_set_config(l4ore_handle_t channel, l4ore_config *new_conf)
{
  DICE_DECLARE_ENV(env);
  l4ore_config old;
 
  if (new_conf == NULL)
      return;

  ore_ore_configure_call(&ore_server, channel, new_conf, &old, &env);
}

