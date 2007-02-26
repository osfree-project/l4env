#include "local.h"
#include <stdlib.h>

#include <l4/names/libnames.h>

int l4ore_send(l4ore_handle_t handle, char *buf, int size)
{
  return descriptor_table[handle].send_func(handle, buf, size);
}

int l4ore_recv_blocking(l4ore_handle_t handle, char **buf, int *size)
{
  return descriptor_table[handle].rx_func_blocking(handle, buf, size);
}

int l4ore_recv_nonblocking(l4ore_handle_t handle, char **buf, int *size)
{
  return descriptor_table[handle].rx_func_nonblocking(handle, buf, size);
}

int l4ore_open(char *device, unsigned char mac[6], l4dm_dataspace_t *send_ds,
               l4dm_dataspace_t *recv_ds, l4ore_config *conf)
{

  l4ore_handle_t ret = ore_do_open(device, mac, send_ds, recv_ds, conf);

  if (ret >= 0)
    {
      // sending via string IPC
      if (send_ds == NULL
          || l4dm_dataspace_equal(*send_ds, L4DM_INVALID_DATASPACE))
        descriptor_table[ret].send_func = ore_send_string;
      else // sending via dataspace
        descriptor_table[ret].send_func = NULL;

      // receiving via string IPC
      if (recv_ds == NULL
          || l4dm_dataspace_equal(*recv_ds, L4DM_INVALID_DATASPACE))
        {
          descriptor_table[ret].rx_func_blocking      = ore_recv_string_blocking;
          descriptor_table[ret].rx_func_nonblocking   = ore_recv_string_nonblocking; 
        }
      else // receiving via dataspace
        {
          descriptor_table[ret].rx_func_blocking      = NULL;
          descriptor_table[ret].rx_func_nonblocking   = NULL;
        }
    }

  return ret;
}

void l4ore_close(l4ore_handle_t handle)
{
  ore_do_close(handle);
}

int ore_lookup_server(void)
{
  // lookup ORe if necessary
  if (l4_thread_equal(ore_server, L4_INVALID_ID))
    {
      if (!names_waitfor_name("ORe", &ore_server, 10000))
        {
          LOG("Could not find ORe server, aborting.");
          return -1;
        }
      LOG("ORe = "l4util_idfmt, l4util_idstr(ore_server));
    }
  return 0;
}

