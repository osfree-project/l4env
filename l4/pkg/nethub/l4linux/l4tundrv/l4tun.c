/*
 *  L4TUN - Universal L4 IP tunnel device driver.
 *  Copyright (C) 2003 Alexander Warg <alexander.warg@os.inf.tu-dresden.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 */

/*
 * The code is based on the TUN/TAP device driver, originally shipped 
 * with linux 2.4. Thanks for the code.
 */

#define L4TUN_VER "0.2"
#define MAX_NUM_TUNS 4

#include <linux/config.h>
#include <linux/module.h>

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/random.h>
#include <asm/uaccess.h>

#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/rtnetlink.h>
#include <linux/spinlock.h>
#include <linux/if.h>
#include <net/protocol.h>
//#include <net/ip.h>
#include <linux/in.h>
#include <linux/if_arp.h>
#include <linux/if_l4tun.h>
#include <linux/version.h>

#include <asm/system.h>
#if !defined(DDE_LINUX)
#include <asm/api/config.h>
#include <asm/l4lxapi/thread.h>
#else
#include <linux/l4lxlib.h>
#endif

#include <l4/util/util.h>
#include <l4/sys/syscalls.h>
#include <l4/nethub/client.h>

#include <asm/e820.h>
#include <linux/sysctl.h>

#if defined(DDE_LINUX)
# include <l4/dde_linux/dde.h>
#endif

#if defined(KBUILD_BASENAME) && (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
#  define LINUX26 1
#endif
#if defined(LINUX_ON_L4)
#  define LINUX24 1
#endif

#ifdef LINUX26
# include <asm/generic/setup.h>
#endif


#if defined(LINUX24)
#  define INITIALIZE_THREAD() do {} while(0)
#elif defined(DDE_LINUX)
#  define INITIALIZE_THREAD() l4dde_process_add_worker()
#elif defined(LINUX26)
#  define INITIALIZE_THREAD() l4x_setup_ti_thread_stack(current_thread_info())
#else
#  error Unsupported environment
#endif

extern inline unsigned decrease_interval( struct l4tun_struct *d )
{
  return d->decrease_interval;
}

extern inline unsigned min_buffer_size( struct l4tun_struct *d )
{
  return d->min_buffer_size;
}

static struct ctl_table devs_table[MAX_NUM_TUNS+2];

static struct ctl_table root_table[] =
{
	{200, "l4tun", NULL, 0, 0600, devs_table},
	{0}
};

static char *tuns[MAX_NUM_TUNS];
static int current_tun = 0;
static struct net_device *esp_tun = 0;

static l4_threadid_t mapper;

static void l4tun_net_init(struct net_device *dev);

#if !defined(DDE_LINUX)
static unsigned long ram_start(void)
{
  int i;
  unsigned long min = (unsigned long)-1;
  for(i = 0; i<e820.nr_map; i++) {
    if( e820.map[i].type == E820_RAM &&
        min >= e820.map[i].addr )
      min = e820.map[i].addr;
  }
  return min;
}

static unsigned long ram_end(void)
{
  int i;
  unsigned long max = 0;
  for(i = 0; i<e820.nr_map; i++) {
    if( e820.map[i].type == E820_RAM &&
        max <= (e820.map[i].addr + e820.map[i].size) )
      max = e820.map[i].addr + e820.map[i].size;
  }
  return max;
}

#else

static unsigned long ram_start(void)
{
  int i;
  unsigned long min = (unsigned long)-1;
  l4_addr_t start, end;
  for(i = 0; !l4dde_mm_kmem_region(i, &start, &end); i++) {
    if (min >= start)
      min = start;
  }
  return min;
}

static unsigned long ram_end(void)
{
  int i;
  unsigned long max = 0;
  l4_addr_t start, end;
  for(i = 0; !l4dde_mm_kmem_region(i, &start, &end); i++) {
    if (max <= end)
      max = end;
  }
  return max;
}
#endif

static struct l4tun_struct *first = NULL;

static struct l4tun_struct *alloc_l4tun(void) 
{
  struct l4tun_struct *tun;
  struct l4tun_struct *tmp;
  struct net_device *ndev; 
  
  /* Allocate new device */
  printk("l4tun: alloc device\n");
  ndev = alloc_netdev(sizeof(struct l4tun_struct), "l4tun%d",
                     l4tun_net_init);

  if (!ndev)
    return 0;

  tun = netdev_priv(ndev);
  memset(tun, 0, sizeof(struct l4tun_struct));
  
  tun->dev = ndev;
  tun->iface.out_ring = &tun->queue;
  tun->iface.in       = L4_INVALID_ID;
  tun->iface.out      = L4_INVALID_ID;
  tun->hub_id   = L4_INVALID_ID;
  tun->snd      = l4_myself();
  tun->attached = 0;
  tun->decrease_interval = 5;
  tun->min_buffer_size   = 1024;
  tun->conf[0].ctl_name     = 150; 
  tun->conf[0].procname     = "dec_interval";
  tun->conf[0].data         = &tun->decrease_interval;
  tun->conf[0].maxlen       = sizeof(int);
  tun->conf[0].mode         = 0660;
  tun->conf[0].child        = NULL;
  tun->conf[0].proc_handler = &proc_dointvec;
  tun->conf[1].ctl_name     = 151; 
  tun->conf[1].procname     = "min_buf_size";
  tun->conf[1].data         = &tun->min_buffer_size;
  tun->conf[1].maxlen       = sizeof(int);
  tun->conf[1].mode         = 0660;
  tun->conf[1].child        = NULL;
  tun->conf[1].proc_handler = &proc_dointvec;
  tun->next     = NULL;

  if(!first)
    first = tun;
  else {
    for(tmp = first; tmp->next!=NULL; tmp = tmp->next) ;
    tmp->next = tun;
  }
  
  return tun;
}

static void free_l4tun(struct l4tun_struct *tun) 
{
  struct l4tun_struct *tmp;

  if(!tun)
    return;

  if(first==tun)
    first = tun->next;
  else {
    for(tmp = first; tmp->next != NULL && tmp->next != tun; tmp = tmp->next) ;
    if(tmp->next)
      tmp->next = tmp->next->next;
  }
  free_netdev(tun->dev);
}

static int __init l4tun_k_param( char *attr )
{
  if(current_tun >= MAX_NUM_TUNS) {
    printk("l4tun: too much l4tun devices requested\n");
    return 0;
  }
  tuns[current_tun++] = attr;
  return 0;
}

__setup("l4tun=",l4tun_k_param);

static 
struct l4tun_struct *l4tun_parse_k_param( char *attr )
{
  int x;
  char *e;
  struct l4tun_struct *tun;

  tun = alloc_l4tun();
  if(!tun) {
    printk(KERN_ERR "l4tun: out of memory\n");
    return NULL;
  }
  for(x=0; attr[x]!=0 && attr[x]!=','; x++ ) {
    tun->hub.name[x] = attr[x];
  }
  tun->hub.name[x] = 0;

  if(attr[x++]==0) {
    printk(KERN_ERR "l4tun: wrong parameter string:"
           " use l4tun=<hubname>,<port>\n");
    free_l4tun(tun);
    return NULL;
  }
  tun->hub.port = simple_strtoul(attr + x, &e, 0);

  if (e && e[0]==',' && e[1]=='x')
    tun->hub.flags = L4TUN_IPSEC_DEV;
  
  printk( KERN_INFO "l4tun: found hub: name='%s' port=%d\n", 
          tun->hub.name,
          tun->hub.port );

  return tun;
}


static void mapper_thread( void *data )
{
  INITIALIZE_THREAD();
  
  nh_region_mapper( (void*)ram_start(), (void*)ram_end() );
}

static inline void sleep_some_time(void)
{
  l4_umword_t d1,d2;
  l4_msgdope_t result;
  l4_ipc_receive( L4_NIL_ID, 0, &d1, &d2,
                  L4_IPC_TIMEOUT(0,0,153,7,0,0),
		  &result );
}

static void txe_irq_thread( void *data )
{
  struct l4tun_struct *tun = *(struct l4tun_struct**)data;
  l4_umword_t d1,d2;
  l4_msgdope_t result;
  l4_threadid_t snd;

  INITIALIZE_THREAD();
  
  l4_ipc_receive( tun->snd, 0, &d1, &d2, L4_IPC_NEVER, &result );

  while(1) {
    l4_ipc_wait( &snd, 0, &d1, &d2, L4_IPC_NEVER, &result );
    printk("L4tun TXE IRQ\n");
    if (tun->dev->flags & IFF_UP)
      /* Otherwise process may sleep forever */
      netif_wake_queue(tun->dev);
    else
      netif_start_queue(tun->dev);
  }
}

static void rcv_thread( void *data )
{
  struct l4tun_struct *tun = *(struct l4tun_struct**)data;
  l4_umword_t id, bytes_rcvd;
  l4_msgdope_t result;
  struct sk_buff *skb;
  unsigned long small_count = 0;
  unsigned long alloc_len = 4096;
  unsigned long len;
  int err;

  INITIALIZE_THREAD();
  
  l4_ipc_receive( tun->snd, 0, &id, &bytes_rcvd, L4_IPC_NEVER, &result );


  while(1) {
    while(1) {
      if (!(skb = alloc_skb(alloc_len + 2, GFP_ATOMIC))) {
        tun->stats.rx_dropped++;
        continue;
      } else
        break;
    }

    skb_reserve(skb, 2);
    skb->dev = tun->dev;
    while (1) {
      unsigned min_buf_size = min_buffer_size(tun);
      unsigned dec_int      = decrease_interval(tun);
      len = alloc_len;
      err = nh_recv( &tun->iface, skb->tail, &len );
      
      if (err == L4_NH_OK) {
        skb_put(skb, len);
	skb->mac.raw = skb->data;
	skb->protocol = __constant_htons(ETH_P_IP);

	tun->stats.rx_packets++;
	tun->stats.rx_bytes += skb->len;

	if (alloc_len > min_buf_size && len <= (alloc_len>>1))
	  small_count++;
	else if (small_count > 0)
	  small_count--;

	if (small_count > dec_int) {
	  small_count = 0;
	  alloc_len = (alloc_len >> 1) & ~(min_buf_size-1);
	  printk("%s: reduce rcv buffer size to %ld\n", tun->name, alloc_len );
	}

	netif_rx(skb);
	break;
      } else if (err == L4_NH_EREALLOC) {
	small_count = 0;
	alloc_len = (len + min_buf_size - 1) & ~(min_buf_size-1);
	kfree_skb(skb);
	break;
      } else if (err == L4_NH_EAGAIN) {
	sleep_some_time();
	continue;
      } else {
	kfree_skb(skb);
	sleep_some_time();
	break;
      }
    }
  }
  kfree_skb(skb);
}


static int contact_hub( struct l4tun_struct *tun )
{
  tun->hub_id = nh_resolve_hub(tun->hub.name);
  if(!l4_is_invalid_id(tun->hub_id)) {
    printk(KERN_INFO "%s: found hub '%s' at id %x.%x\n",
           tun->name, tun->hub.name, 
           tun->hub_id.id.task, tun->hub_id.id.lthread);
    return 1;
  } 
    
  return 0;
}

static int connect_hub( struct l4tun_struct *tun )
{
  int err;
  l4_msgdope_t result;

  printk("%s: connecting to nethub\n", tun->name);

#if 0
  printk("%s: %x map memory from 0x%llx to 0x%llx to nethub\n",
         tun->name, tun->rcv.id.task, ram_start(), ram_end() );
#endif

  if(!tun) 
    return 0;
  
  if(l4_is_invalid_id(tun->hub_id))
    if(!contact_hub(tun))
      return 0;
  tun->snd = l4_myself();

  tun->rcv = l4lx_thread_create(rcv_thread,
                                NULL,
                                &tun, sizeof(struct l4tun_struct*),
                                0x90,
                                "L4Tun rcv");
  tun->iface.txe_irq = l4lx_thread_create(txe_irq_thread,
                                NULL,
                                &tun, sizeof(struct l4tun_struct*),
                                0x90,
                                "L4Tun txe");

  tun->iface.mapper           = mapper;
  tun->iface.shared_mem_start = (void*)ram_start();
  tun->iface.shared_mem_end   = (void*)ram_end();
  tun->iface.rcv_thread       = tun->rcv;
  
  if (nh_if_open(tun->hub_id, tun->hub.port, 
		 &tun->iface) != NH_OK)
    {
      printk("%s: could not connect device to l4 hub\n", 
                 tun->name);
      goto connect_error;
    }

#if 0
  printk("%s: hub at %x.%x; %x.%x\n", tun->name, 
         tun->in.id.task, tun->in.id.lthread,
         tun->out.id.task, tun->out.id.lthread );
#endif

  err = l4_ipc_send( tun->rcv, 0, 0, 0, L4_IPC_NEVER, &result );  
  if(err) {
    printk(KERN_ERR "%s: rcv handshake failed\n",tun->name);
    goto connect_error;
  }
  
  err = l4_ipc_send( tun->iface.txe_irq, 0, 0, 0, L4_IPC_NEVER, &result );  
  if(err) {
    printk(KERN_ERR "%s: txe handshake failed\n",tun->name);
    goto connect_error;
  }

  tun->attached = 1;
  return 1;

connect_error:    
  l4lx_thread_shutdown(tun->rcv);
  return 0;
}

/* Network device part of the driver */

/* Net device open. */
static int l4tun_net_open(struct net_device *dev)
{
  struct l4tun_struct *tun = (struct l4tun_struct*)dev->priv;
  if(!connect_hub(tun))
    return -EBUSY;

  memset( &tun->queue, 0, sizeof(tun->queue));

  if (tun->hub.flags & L4TUN_IPSEC_DEV) {
    esp_tun = dev;
#if 0
    tun->ipsec = dev_get_by_name("eth0");
    printk("l4tun: ipsec dev eth0 @%p\n",tun->ipsec);
#endif
  }
  return 0;
}

/* Net device close. */
static int l4tun_net_close(struct net_device *dev)
{
  struct l4tun_struct *tun = (struct l4tun_struct*)dev->priv;
  tun->hub_id = L4_INVALID_ID;
  l4lx_thread_shutdown(tun->rcv);
  l4lx_thread_shutdown(tun->iface.txe_irq);
  tun->rcv = L4_INVALID_ID;
  tun->iface.txe_irq = L4_INVALID_ID;
  if (esp_tun == dev)
    esp_tun = 0;
  return 0;
}

static void my_free_skb( struct sk_buff **s )
{
  if (*s) {
    kfree_skb(*s);
    *s= 0;
  }
}

/* Net device start xmit */
static int l4tun_net_xmit(struct sk_buff *skb, struct net_device *dev)
{
  struct l4tun_struct *tun = (struct l4tun_struct *)dev->priv;
  int rc = 0;
  
  unsigned long irq_state;
  
  //printk("%s: l4tun_net_xmit %d\n", tun->name, skb->len);
#if 0
    printk("%s: send buffer at %p, len=%x\n", 
           tun->name, skb->data, skb->len );
#endif

  spin_lock_irqsave(&tun->lock, irq_state);
  if (nh_send( &tun->iface, skb->data, skb ) == L4_NH_OK) {
    tun->stats.tx_packets++;
    tun->stats.tx_bytes += skb->len;
    spin_unlock_irqrestore(&tun->lock, irq_state);
  } else {
    tun->stats.tx_errors++;
    spin_unlock_irqrestore(&tun->lock, irq_state);
//    kfree_skb(skb);
    netif_stop_queue(dev);
    printk("L4tun: stop TX queue\n");
    rc = 1;
  }

  spin_lock_irqsave(&tun->lock, irq_state);
  nh_for_each_empty_slot(&tun->queue, (Nh_slot_func*)my_free_skb);
  spin_unlock_irqrestore(&tun->lock, irq_state);
  
  return rc;
}

static int ipsec_rcv( struct sk_buff *skb )
{
  //  printk("L4TUN-ESP: handle packet\n");
  if (skb == NULL) {
    printk("ipsec_rcv: got NULL skb\n");
    return 0;
  }

  if (skb->data == NULL) {
    printk("ipsec_rcv: got NULL skb->data\n");
    kfree_skb(skb);
    return 0;
  }

  if (skb->data == skb->h.raw) {
    skb_push(skb, skb->h.raw - skb->nh.raw);
  }

  if (skb_is_nonlinear(skb)) {
    if (skb_linearize(skb, GFP_ATOMIC) != 0) {
      kfree_skb(skb);
      return 0;
    }
  }
  if (esp_tun)
    return l4tun_net_xmit( skb, esp_tun );
  else
    kfree_skb(skb);

  return 0;
}

static struct net_device_stats *l4tun_net_stats(struct net_device *dev)
{
  struct l4tun_struct *tun = (struct l4tun_struct *)dev->priv;
  return &tun->stats;
}

/* Initialize net device. */
void l4tun_net_init(struct net_device *dev)
{
  printk("l4tun_net_init\n");
  
  SET_MODULE_OWNER(dev);
  dev->open = l4tun_net_open;
  dev->hard_start_xmit = l4tun_net_xmit;
  dev->stop = l4tun_net_close;
  dev->get_stats = l4tun_net_stats;
  dev->destructor = free_netdev;
	
  /* Point-to-Point TUN Device */
  dev->hard_header_len = 0;
  dev->addr_len = 0;
  dev->mtu = 1500;
  
  /* Type PPP seems most suitable */
  dev->type = ARPHRD_PPP; 
  dev->flags = IFF_POINTOPOINT | IFF_NOARP | IFF_MULTICAST;
  dev->tx_queue_len = 10;
}


static int create_net_dev( struct l4tun_struct *tun )
{
  int err;
  unsigned i;

  if(!tun)
    return 0;

  err = -EINVAL;
  
  if (strchr(tun->dev->name, '%')) {
      err = dev_alloc_name(tun->dev, tun->dev->name);
      if (err < 0)
	goto failed;
  }

  tun->name = tun->dev->name;

  /* Set dev type */
  if ((err = register_netdevice(tun->dev)))
    goto failed;

#if !defined(LINUX26)
  MOD_INC_USE_COUNT;
#endif
  
  tun->name = tun->dev->name;
  tun->rcv = L4_INVALID_ID;
  
  for (i=0; i<MAX_NUM_TUNS; i++) {
    if (devs_table[i].ctl_name == 0) {
      devs_table[i].ctl_name = 201 + i;
      devs_table[i].procname = tun->name;
      devs_table[i].data     = NULL;
      devs_table[i].maxlen   = 0;
      devs_table[i].mode     = 0600;
      devs_table[i].child    = tun->conf;
      devs_table[i+1].ctl_name = 0;
      break;
    }
  }

  return 0;

failed:
  return err;
}


#if !defined(LINUX26)
static struct inet_protocol esp_protocol =
{
  ipsec_rcv,
  NULL,
  0,
  IPPROTO_ESP,
  0,
  NULL,
  "ESP"
};
#else
static struct net_protocol esp_protocol =
{
  ipsec_rcv,
  NULL,
  0,
};
#endif

static struct ctl_table_header *l4tun_device_sysctl_header;


#ifdef LINUX26
static 
int l4tun_control(ctl_table *table, int write, struct file *filp, 
                  void __user *buf, size_t *len, loff_t *off)
#else
static 
int l4tun_control(ctl_table *table, int write, struct file *filp, 
                  void *buf, size_t *len)
#endif
{
  char buffer[256];
  int tmp, l;
  struct l4tun_struct *tun;
  
  if (!write)
    return -EINVAL;
  else
    {
      if (*len>255) 
	l = 255;
      else
	l = *len;

      tmp = copy_from_user(buffer, buf, l);
      
      if (tmp!=0)
	return -EINVAL;

      buffer[l] = 0;
      tun = l4tun_parse_k_param(buffer);
      if (tun)
	{
	  rtnl_lock();
  	  create_net_dev(tun);
	  rtnl_unlock();
	}
      else
	return -ENOMEM;

      return 0;
    }

  return -EINVAL;
}

int __init l4tun_init(void)
{
  struct l4tun_struct *tun;
  int x;
  unsigned i;
  printk(KERN_INFO "Universal L4TUN device driver %s " 
         "(C)2003 Alexander Warg\n", L4TUN_VER);


  for (i=0; i<=MAX_NUM_TUNS+1; i++)
    devs_table[i].ctl_name = 0;

  devs_table[0].ctl_name = 201;
  devs_table[0].procname = "control";
  devs_table[0].data     = NULL;
  devs_table[0].maxlen   = 0;
  devs_table[0].mode     = 0660;
  devs_table[0].child    = NULL;
  devs_table[0].proc_handler = &l4tun_control;

  devs_table[1].ctl_name = 0;

  
  for(x=0; x<current_tun; x++)
    l4tun_parse_k_param(tuns[x]);

  rtnl_lock();
  for(tun = first; tun!=0; tun=tun->next) {
    create_net_dev(tun);
  }
  rtnl_unlock();
  
  l4tun_device_sysctl_header = register_sysctl_table(root_table, 0);
  
  if (!l4tun_device_sysctl_header)
    return -1;

  mapper = l4lx_thread_create(mapper_thread,
                              NULL, NULL, 0,
                              0xb0,
                              "L4Tun mapper");
# if !defined(LINUX26)
  inet_add_protocol( &esp_protocol );
# else
  inet_add_protocol(&esp_protocol, IPPROTO_ESP);
#endif
  return 0;
}

void l4tun_cleanup(void)
{
# if !defined(LINUX26)
  inet_del_protocol(&esp_protocol);
# else
  inet_del_protocol(&esp_protocol, IPPROTO_ESP);
# endif
  unregister_sysctl_table(l4tun_device_sysctl_header);
  l4tun_device_sysctl_header = NULL;
}

module_init(l4tun_init);
module_exit(l4tun_cleanup);
MODULE_LICENSE("GPL");

