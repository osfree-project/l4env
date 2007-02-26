#ifndef __ORE_TYPES_H
#define __ORE_TYPES_H

#include <l4/sys/types.h>
#include <l4/dm_generic/types.h>
#include <l4/ore/ore-dsi.h>

// this is the handle definition for L4v2 --> one should be able to
// adapt this easily to handle e.g. L4.Sec-style badges
typedef l4_threadid_t	l4ore_handle_t;

/*!\brief ORe connection configuration
 * \ingroup management
 *
 * Used to configure an ORe connection during l4ore_open() and later on
 * using l4ore_get_config() and l4ore_set_config().
 */
typedef struct ore_connection_configuration
{
    int rw_debug;                   //!< set to 1 if debugging is desired
    int rw_broadcast;               //!< set to 1 if you want to receive broadcast packets
    int rw_active;                  //!< if set 0, all packets incoming for this connection
                                    // are discarded
    int ro_keep_device_mac;         //!< set to 1 on open if you want to keep the original
                                    //!< device mac.
    int ro_irq;                     //!< irq no. of the belonging NIC
    int ro_mtu;                     //!< mtu of the belonging NIC
    l4dm_dataspace_t ro_send_ds;                //!< send data area (DSI)
    l4dm_dataspace_t ro_send_ctl_ds;            //!< send control area (DSI)
    dsi_socket_ref_t  ro_send_client_socketref; //!< the clients socket for sending packets
    dsi_socket_ref_t  ro_send_ore_socketref;    //!< ORe's socket for outgoing packets
    l4dm_dataspace_t ro_recv_ds;                //!< receive data area (DSI)
    l4dm_dataspace_t ro_recv_ctl_ds;            //!< receive control area (DSI)
    dsi_socket_ref_t  ro_recv_client_socketref; //!< the client's socket for receiving packets
    dsi_socket_ref_t  ro_recv_ore_socketref;    //!< ORe's socket for incoming packets
    char ro_orename[16];            //!< name of the ORe instance we connect to
} l4ore_config;

#define LOG_CONFIG(conf)    {                \
                                LOG("conf->debug     = %d", (conf).rw_debug);           \
                                LOG("conf->broadcast = %d", (conf).rw_broadcast);       \
                                LOG("conf->active    = %d", (conf).rw_active);          \
                                LOG("conf->keep_mac  = %d", (conf).ro_keep_device_mac); \
                                LOG("conf->irq       = %d", (conf).ro_irq);             \
                                LOG("conf->mtu       = %d", (conf).ro_mtu);             \
                                LOG("conf->send_ds   = DS %d", (conf).ro_send_ds.id);       \
                                LOG("conf->send_ctl  = DS %d", (conf).ro_send_ctl_ds.id);   \
                                LOG("conf->recv_ds   = DS %d", (conf).ro_recv_ds.id);       \
                                LOG("conf->recv_ctl  = DS %d", (conf).ro_recv_ctl_ds.id);   \
                                LOG("conf->send_client_socket = %d", (conf).ro_send_client_socketref.socket);   \
                                LOG("conf->send_ore_socket = %d", (conf).ro_send_ore_socketref.socket);   \
                                LOG("conf->recv_client_socket = %d", (conf).ro_recv_client_socketref.socket);   \
                                LOG("conf->recv_ore_socket = %d", (conf).ro_recv_ore_socketref.socket);   \
                                LOG("conf->orename = %s", (conf).ro_orename); \
                            }

#define LOG_SOCKETREF(s)   \
    LOG("socketid = %d", (s)->socket);  \
    LOG("worker = "l4util_idfmt, l4util_idstr((s)->work_th));    \
    LOG("sync   = "l4util_idfmt, l4util_idstr((s)->sync_th));    \
    LOG("eventh = "l4util_idfmt, l4util_idstr((s)->event_th));   

#define LOG_SOCKET(s)   \
    LOG("socket id = %d", (s)->socket_id);         \
    LOG("data ds = DS %d", (s)->data_ds.id);    \
    LOG("data is at %p", (s)->data_area);

#define LOG_PACKET(p)   \
    LOG("packet number = %d", (p)->no);

#define mac_fmt       "%02X:%02X:%02X:%02X:%02X:%02X"
#define mac_str(mac)  (mac)[0],(mac)[1],(mac)[2],(mac)[3],(mac)[4],(mac)[5]

#define LOG_MAC_s(cond, str, mac)  LOGd(cond, "%s " mac_fmt, (str), mac_str(mac))

#define LOG_MAC(cond, mac) LOG_MAC_s(cond, "MAC = ", (mac))

#define L4ORE_DEFAULT_INITIALIZER  { 0, 0, 1, 0, 0, 0,                  \
                        L4DM_INVALID_DATASPACE, L4DM_INVALID_DATASPACE, \
                        {-1, L4_INVALID_ID, L4_INVALID_ID, L4_INVALID_ID},  \
                        {-1, L4_INVALID_ID, L4_INVALID_ID, L4_INVALID_ID},  \
                        L4DM_INVALID_DATASPACE, L4DM_INVALID_DATASPACE, \
                        {-1, L4_INVALID_ID, L4_INVALID_ID, L4_INVALID_ID},  \
                        {-1, L4_INVALID_ID, L4_INVALID_ID, L4_INVALID_ID},  \
                        {'O', 'R', 'e', 0, 0, 0, 0, 0, 0, 0},    \
                    }

#define L4ORE_INVALID_INITIALIZER  { -1,-1,-1,-1,-1,-1,                  \
                        L4DM_INVALID_DATASPACE, L4DM_INVALID_DATASPACE, \
                        {-1, L4_INVALID_ID, L4_INVALID_ID, L4_INVALID_ID},  \
                        {-1, L4_INVALID_ID, L4_INVALID_ID, L4_INVALID_ID},  \
                        L4DM_INVALID_DATASPACE, L4DM_INVALID_DATASPACE, \
                        {-1, L4_INVALID_ID, L4_INVALID_ID, L4_INVALID_ID},  \
                        {-1, L4_INVALID_ID, L4_INVALID_ID, L4_INVALID_ID},  \
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},    \
                    }

#define L4ORE_DEFAULT_CONFIG ((l4ore_config)L4ORE_DEFAULT_INITIALIZER)
#define L4ORE_INVALID_CONFIG ((l4ore_config)L4ORE_INVALID_INITIALIZER)

/*!\brief Check if config is valid.
 * \ingroup management
 */
L4_INLINE int l4ore_is_invalid_config(l4ore_config);
L4_INLINE int l4ore_is_invalid_config(l4ore_config c)
{
  return (c.rw_debug == -1);
}

#endif
