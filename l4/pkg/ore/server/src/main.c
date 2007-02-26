/*************************************************
 * ORe - Oshkosh Resurrection                    *
 *                                               *
 * Bjoern Doebel <doebel@os.inf.tu-dresden.de>   *
 * 2005-08-10                                    *
 *************************************************/

#include <linux/netdevice.h>

#include <l4/sys/types.h>
#include <l4/util/l4_macros.h>
#include <l4/util/util.h>
#include <l4/util/parse_cmd.h>
#include <l4/rmgr/librmgr.h>
#include <l4/dde_linux/dde.h>
#include <l4/names/libnames.h>
#include <l4/sys/syscalls.h>
#include <l4/util/getopt.h>
#include <l4/util/kip.h>

#include <stdlib.h>

#include "ore-local.h"

static const char *_oreName = "ORe";
/* MAC addresses are by default 04:EA:DD:00:00:xx; where xx is the client
 * handle. (04EADD --> OREADD)
 * FIXME no global vars, please */
unsigned char global_mac_address_head[4] = { 0x04, 0xEA, 0xDD, 0x00};
static   int  use_omega0;

/* Our version of CORBA_alloc().
 *
 * This one is tricky: Dice-generated code will allocate buffers for receiving 
 * packets, because ore_recv() is marked "prealloc" within the ORe idl. On the
 * server side this is however unnecessary, because we will never use this buffer
 * to send a packet to a client, but we use the data area of an skb allocated by
 * the NIC driver instead.
 *
 * The Dice code's malloc/free-policy may lead to contention of our whole kernel
 * memory, if a client issues several requests one after another. In this case
 * we will run out of memory. This may be circumvented, if we do not allocate
 * memory here but return a NULL pointer. This is safe as long as we do not use 
 * the Dice buffer.
 *
 * A special case is Dice's first call to CORBA_alloc(). This will allocate 
 * memory for the real receive buffer and is truly necessary. As we know how
 * large this buffer is, we kmalloc() memory if someone calls CORBA_alloc() with
 * the right size.
 *
 * However, this may result in memory being allocated if later Dice code wants
 * to have a buffer of this size, but this incident will not happen that often,
 * so that we may live with that.
 *
 * FIXME: Dice should be altered to distinguish client-side and server-side
 *        prealloc.
 */
void *CORBA_alloc(unsigned long size)
{
  if (size == ORE_CONFIG_MAX_BUF_SIZE)
    return kmalloc(size, GFP_KERNEL);

  return NULL;
}

/* CORBA_free() - not that tricky. We only free ptr if it is != NULL. */
void CORBA_free(void *ptr)
{
  if (ptr == NULL)
    return;

  kfree(ptr);
}

/** Copy mac address from command line */
static void copy_mac(int id, const char *arg, int num)
{
  unsigned long tmp = strtoul(optarg, NULL, 16);

  global_mac_address_head[3] = (unsigned char)(tmp & 0xff);
  global_mac_address_head[2] = (unsigned char)(tmp >> 8 & 0xff);
  global_mac_address_head[1] = (unsigned char)(tmp >> 16 & 0xff);
  global_mac_address_head[0] = (unsigned char)(tmp >> 24);
}

/* Basic initialization. */
int main(int argc, const char **argv)
{
  int ret, ux;

  // setup server environment
  DICE_DECLARE_SERVER_ENV(env);
  env.malloc = (dice_malloc_func)CORBA_alloc;
  env.free   = (dice_free_func)CORBA_free;

  // using this function induces two benefits
  // (1) it is shorter and better to read than getopt_long()
  // (2) it provides a help screen if the binary is called with --help
  if (parse_cmdline(&argc, &argv,
	            'm', "mac", "mac address",
		    PARSE_CMD_FN_ARG, 0, &copy_mac,
		    'o', "omega0", "use omega0 for IRQ handling",
		    PARSE_CMD_SWITCH, 1, &use_omega0,
		    0, 0))
    return 1;

  ore_main_server = l4_myself();

  rmgr_init();

  l4util_kip_map();
  ux = l4util_kip_kernel_is_ux();
  l4util_kip_unmap();

  // initialize linux emulation library
  LOGd(ORE_DEBUG_INIT, "memsize = %d", ORE_LINUXEMUL_MEMSIZE);
  ret = init_emulation(ORE_LINUXEMUL_MEMSIZE, use_omega0, custom_irq_handler, ux);
  LOGd(ORE_DEBUG_INIT, "init_emulation: %d (%s)", ret, l4env_strerror(-ret));

  // initialize network devices
  ret = open_network_devices();
  LOG("Initialized %d network devices.", ret);

  // list devices at debug time
  if (ORE_DEBUG)
    list_network_devices();

  init_connection_table();

  // register
  LOG("Registering at names...");
  ret = names_register(_oreName);
  if (!ret)
    LOG_Error("Could not register at names.");

  LOGd(ORE_DEBUG, "registered at names: %d", ret);

  LOG("Ready to service.");
  ore_server_ore_internal_server_loop(&env);

  LOG("Loop returned.\n");
  l4_sleep_forever();

  return 0;
}
