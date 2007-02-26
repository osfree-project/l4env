#ifndef __ORE_TYPES_H
#define __ORE_TYPES_H

#include <l4/sys/types.h>

// this is the handle definition for L4v2 --> one should be able to
// adapt this easily to handle e.g. L4.Sec-style badges
typedef l4_int32_t	l4ore_handle_t;

typedef struct ore_connection_configuration
{
    int rw_debug;               // set to 1 if debugging is desired
    int rw_broadcast;           // set to 1 if you want to receive broadcast packets
    int rw_active;              // if set 0, all packets incoming for this connection
                                // are discarded
    int ro_keep_device_mac;     // set to 1 on open if you want to keep the original
                                // device mac.
    int ro_irq;                 // irq no. of the belonging NIC
    int ro_mtu;                 // mtu of the belonging NIC
} l4ore_config;

#define LOG_CONFIG(conf)    {                \
                                LOG("conf->debug     = %d", conf.rw_debug);        \
                                LOG("conf->broadcast = %d", conf.rw_broadcast);    \
                                LOG("conf->active    = %d", conf.rw_active);       \
                                LOG("conf->keep_mac  = %d", conf.ro_keep_device_mac); \
                                LOG("conf->irq       = %d", conf.ro_irq);          \
                                LOG("conf->mtu       = %d", conf.ro_mtu);          \
                            }

#define L4ORE_DEFAULT_INITIALIZER  {0,0,1,0,0,0}
#define L4ORE_INVALID_INITIALIZER  {-1,-1,-1,-1,-1,-1}

#define L4ORE_DEFAULT_CONFIG ((l4ore_config)L4ORE_DEFAULT_INITIALIZER)
#define L4ORE_INVALID_CONFIG ((l4ore_config)L4ORE_INVALID_INITIALIZER)

int l4ore_is_invalid_config(l4ore_config);
L4_INLINE int l4ore_is_invalid_config(l4ore_config c)
{
  return (c.rw_debug == -1);
}

#endif
