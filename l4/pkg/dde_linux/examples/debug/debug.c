/* $Id$ */
/*****************************************************************************/
/**
 * \file    dde_linux/examples/debug/debug.c
 *
 * \brief   DDE Example (debugging purposes)
 *
 * \author  Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4 */
#include <l4/util/macros.h>
#include <l4/util/getopt.h>
#include <l4/generic_io/libio.h>
#include <l4/thread/thread.h>
#include <l4/dde_linux/dde.h>

/* Linux */
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/tqueue.h>

/* LibC */
#include <stdlib.h>

char LOG_tag[9] = "-DDESRV-\0";

#define FINAL_TAG "dde dbg \0"

/*****************************************************************************/

#if 1
#define PCI_DEV(dev) \
        LOG("DEBUG:\n" \
            "pci_dev  bus:devfn          %x:%02x.%x\n" \
            "         vendor               %04x\n" \
            "         device               %04x\n" \
            "         class            %08x\n" \
            "         slotname          %s\n" \
            "         irq                     %x\n" \
            "         res0    %08lx-%08lx (%08lx)\n" \
            "         res1    %08lx-%08lx (%08lx)", \
            (dev)->bus->number, PCI_SLOT((dev)->devfn), PCI_FUNC((dev)->devfn), \
            (dev)->vendor, (dev)->device, \
            (dev)->class, (dev)->slot_name, (dev)->irq, \
            (dev)->resource[0].start, (dev)->resource[0].end, (dev)->resource[0].flags, \
            (dev)->resource[1].start, (dev)->resource[1].end, (dev)->resource[1].flags);
#else
#define PCI_DEV(x) do {} while (0)
#endif

static int _pciinit = 0;

/*****************************************************************************/
/** self-made for_each_pci_dev */
/*****************************************************************************/
static int pci_devs_flag = 0;
static void debug_all_pci_devs(void)
{
  int err;
  struct pci_dev *dev = NULL;

  if (!_pciinit)
    {
      if ((err=l4dde_pci_init()))
        {
          LOG_Error("initializing pci (%d)", err);
          return;
        }
      ++_pciinit;
    }

  for (;;)
    {
      dev = pci_find_device(PCI_ANY_ID, PCI_ANY_ID, dev);

      if (!dev)
        break;
      PCI_DEV(dev);
    }
}

/*****************************************************************************/
/** module init */
/*****************************************************************************/
static int __es1371_probe(struct pci_dev *pcidev, const struct pci_device_id *pciid)
{
  LOG("DEBUG: __es1371_probe called (devid=%x)", pcidev->device);

  return 0;
}

static void __es1371_remove(struct pci_dev *dev)
{
  LOG("DEBUG: __es1371_remove called");
}

#ifndef PCI_DEVICE_ID_ENSONIQ_CT5880
#define PCI_DEVICE_ID_ENSONIQ_CT5880 0x5880
#endif

static int pci_mod_flag = 0;
static void debug_pci_module_init(void)
{
  int err;

  struct pci_device_id id_table[] = {
    {PCI_VENDOR_ID_ENSONIQ, PCI_DEVICE_ID_ENSONIQ_ES1371, PCI_ANY_ID, PCI_ANY_ID, 0, 0},
    {PCI_VENDOR_ID_ENSONIQ, PCI_DEVICE_ID_ENSONIQ_CT5880, PCI_ANY_ID, PCI_ANY_ID, 0, 0},
    {PCI_VENDOR_ID_ECTIVA, PCI_DEVICE_ID_ECTIVA_EV1938, PCI_ANY_ID, PCI_ANY_ID, 0, 0},
    {0,}
  };
  struct pci_driver es1371_driver = {
    name:"es1371",
    id_table:id_table,
    probe:__es1371_probe,
    remove:__es1371_remove
  };

  if (!_pciinit)
    {
      if ((err=l4dde_pci_init()))
        {
          LOG_Error("initializing pci (%d)", err);
          return;
        }
      ++_pciinit;
    }

  LOG("DEBUG: version v0.0 time " __TIME__ " " __DATE__ );
  err = pci_module_init(&es1371_driver);
  LOG("DEBUG: pci_module_init() returns ");
  if (err)
    if (err == -19)
      LOG("ENODEV");
    else
      LOG("%d", err);
  else
   LOG("OK");
}

/*****************************************************************************/
/** memory */
/*****************************************************************************/
static int malloc_flag = 0;

static void debug_malloc(void)
{
  void *p0, *p1, *p2;
  int i;
  l4_addr_t begin, end;

  /* simple malloc/free pairs */
  LOG("DEBUG: Allocating ...\n");
  LOG("DEBUG: (1) %d bytes kmem available", l4dde_mm_kmem_avail());
  p0 = kmalloc(1024, GFP_KERNEL); Assert(p0);
  kfree(p0);
  p0 = kmalloc(128, GFP_KERNEL); Assert(p0);
  p1 = kmalloc(0x2000, GFP_KERNEL); Assert(p1);
  LOG("DEBUG: (2) %d bytes kmem available", l4dde_mm_kmem_avail());
  kfree(p1);
  kfree(p0);
  p0 = kmalloc(128, GFP_KERNEL); Assert(p0);
  p1 = kmalloc(0x2000, GFP_KERNEL); Assert(p1);
  p2 = kmalloc(100 * 1024, GFP_KERNEL); Assert(p2);
  LOG("DEBUG: (3) %d bytes kmem available", l4dde_mm_kmem_avail());
  kfree(p2);
  kfree(p1);
  kfree(p0);
  LOG("DEBUG: (4) %d bytes kmem available", l4dde_mm_kmem_avail());

  /* alloc too much */
  LOG("DEBUG: Trigger Out of Memory ...");
  p0 = kmalloc(1024*1024, GFP_KERNEL); Assert(p0);
  LOG("DEBUG: (5) %d bytes kmem available", l4dde_mm_kmem_avail());

  /* test regions */
  for (i=0; !l4dde_mm_kmem_region(i, &begin, &end); i++)
    LOG_printf("kregions[%d]: %p-%p\n", i, (void *)begin, (void *)end);
}

/*****************************************************************************/
/** IRQ */
/*****************************************************************************/
static int free_me = 0;
static void debug_irq_handler(int irq, void *id, struct pt_regs *pt)
{
  static int count = 100;
  static int runs = 2;
  static unsigned long old = 0;

  if (!in_irq())
    Panic("DEBUG: in_irq() not set in interrupt handler");

  if (in_softirq())
    Panic("DEBUG: in_softirq() set in interrupt handler");

  if (!runs)
    free_me = 1;

  /* every 100 IRQs */
  if (!count)
    {
      LOG("DEBUG: -> 100 (%ld)", jiffies - old);
      count = 100;
      old = jiffies;
      runs--;
    }
  count--;
}

static int irq_flag = 0;
static int irq_num = 0;
static void debug_irq(void)
{
  int err;

  if ((err=l4dde_irq_init()))
    {
      LOG_Error("initializing irqs (%d)", err);
      return;
    }

  err = request_irq(irq_num, debug_irq_handler, 0, "haha", 0);
  if (err)
    LOG("DEBUG: request_irq failed (%d)", err);

  while (!free_me)
    l4thread_sleep(100);

  free_irq(irq_num, 0);
}

/*****************************************************************************/
/** jiffies */
/*****************************************************************************/
static int jiffies_flag = 0;
static int jiffies_num = 10;
static void debug_jiffies(void)
{
  LOG("DEBUG: HZ = %lu (%p)", HZ, &HZ);

  while (jiffies_num)
    {
      LOG("DEBUG: %lu jiffies (%p) since startup", jiffies, &jiffies);
      l4thread_sleep(1000);
      jiffies_num--;
    }
}

/*****************************************************************************/
/** locking */
/*****************************************************************************/
static int lock_flag = 0;
static int lock_num = 5;
static int child_sleep = 100;
static int parent_sleep = 250;

static spinlock_t grabme = SPIN_LOCK_UNLOCKED;

static void __lock_grabbing(void *sleep)
{
  int sleep_time = *(int *) sleep;
  unsigned long flags;
  unsigned long t0, t1;
  int child = 0;

  if (sleep == &child_sleep)
    {
      /* child */
      if (l4thread_started(NULL))
        l4thread_exit();
      child = 1;
    }

  for (;;)
    {
      spin_lock_irqsave(&grabme, flags);

      if (!spin_is_locked(&grabme))
        Panic("DEBUG: locked spinlock is NOT locked");

      t0 = jiffies;
      l4thread_sleep(5 * sleep_time);
      t1 = jiffies;

      spin_unlock_irqrestore(&grabme, flags);

      LOG("DEBUG: %6s| from %5ld\n"
          "       %6s|   to %5ld (%ld jiffies)",
          child ? "child" : "parent", t0, "", t1, t1 - t0);

      if (child) --lock_num;

      l4thread_sleep(sleep_time);

      if (!lock_num) break;
    }

  if (child) l4thread_exit();
}

static int __fork(void)
{
  l4thread_t tid;

  /* create child thread */
  tid = l4thread_create((l4thread_fn_t) __lock_grabbing,
                        (void *) &child_sleep, L4THREAD_CREATE_SYNC);

  return tid ? 0 : -1;
}

static void debug_spinlock(void)
{

  if (__fork())
    return;

  __lock_grabbing(&parent_sleep);
}

/*****************************************************************************/
/** timers */
/*****************************************************************************/
static int timer_flag = 0;
static int timers_expired = 0;
#define TIMER_COOKIE 0x13131313

static void __timer_func(unsigned long i)
{
  if (!in_interrupt())
    Panic("DEBUG: in_interrupt() not set in timer handler");

  if (!i)
    timers_expired++;
  if (i == TIMER_COOKIE)
    /* bug triggered */
    LOG("DEBUG: Ouch, found cookie data (0x%lx) @ %ld jiffies", i, jiffies);
  else
    /* normal processing */
    LOG("DEBUG: timer_func [%ld] @ %ld jiffies", i, jiffies);
}

static void debug_timers(void)
{
  int err;
  struct timer_list t0, t1, t2, t3;

  if ((err=l4dde_time_init()))
    {
      LOG_Error("initializing time (%d)", err);
      return;
    }

  timers_expired = 0;

  init_timer(&t0);
  init_timer(&t1);
  init_timer(&t2);
  init_timer(&t3);
  t0.function = t1.function = t2.function = t3.function = __timer_func;

  t0.expires = jiffies + 100;
  t0.data = 3UL;
  t1.expires = jiffies + 200;
  t1.data = 2UL;
  t2.expires = jiffies + 300;
  t2.data = 1UL;
  t3.expires = jiffies + 400;
  t3.data = 0UL;

  LOG("DEBUG: expiration @ (%ld, %ld, %ld, %ld)",
      t0.expires, t1.expires, t2.expires, t3.expires);

  add_timer(&t1);
  add_timer(&t2);
  add_timer(&t0);
  add_timer(&t3);

  LOG("DEBUG: waiting for timer expiration ...");

  while (!timers_expired)
    l4thread_sleep(100); /* wait for latest timer expiration as timers
                            are on stack !!! */

  /* now trigger bug #1 in old dde version:
   * remove last timer while we're waiting for it
   */
  LOG("\nDEBUG: trigger known bug #1 in old version");

  timers_expired = 0;

  init_timer(&t0);
  t0.function = __timer_func;

  t0.expires = jiffies + 500;
  t0.data = 0UL;

  LOG("DEBUG: expiration @ (%ld)", t0.expires);

  add_timer(&t0);

  l4thread_sleep(100);

  del_timer(&t0);

  /* now t0 is invalid, but time.c still used it ... */

  t0.expires = 0;          /* be sure it will be processed */
  t0.data = TIMER_COOKIE;  /* place a cookie */
  t0.list.next = &t1.list;
  t0.list.prev = &t1.list; /* now the "synthetic" _list entry_
                              is valid again */

  LOG("DEBUG: waiting for timer expiration ... (will wait a while)");

  l4thread_sleep(6000);

  /* now trigger bug #2 in old dde version:
   * timer expired before examination
   */
  LOG("\nDEBUG: trigger known bug #2 in old version");

  timers_expired = 0;

  init_timer(&t0);
  t0.function = __timer_func;

  t0.expires = jiffies - 2;
  t0.data = 0UL;

  LOG("DEBUG: expiration @ (%ld)", t0.expires);

  add_timer(&t0);

  while (!timers_expired)
    l4thread_sleep(100); /* wait for latest timer expiration as timers
                            are on stack !!! */
}

/*****************************************************************************/
/** softirqs */
/*****************************************************************************/
static int softirq_flag = 0;

static void __softirq_func(unsigned long i)
{
  if (!in_softirq())
    Panic("DEBUG: in_softirq() not set in softirq handler");

  if (in_irq())
    Panic("DEBUG: in_irq() set in softirq handler");

  LOG("DEBUG: softirq [%ld] @ %ld jiffies (lthread %0x)",
      i, jiffies, l4thread_myself());
}

DECLARE_TASKLET(low0, __softirq_func, 0);
DECLARE_TASKLET(low1, __softirq_func, 1);
DECLARE_TASKLET(low2, __softirq_func, 2);

DECLARE_TASKLET(hi0, __softirq_func, 1000);
DECLARE_TASKLET(hi1, __softirq_func, 1001);
DECLARE_TASKLET(hi2, __softirq_func, 1002);

static void debug_softirq(void)
{
  int err;

  if ((err=l4dde_softirq_init()))
    {
      LOG_Error("initializing softirqs (%d)", err);
      return;
    }
  /* 3 lo : 1 hi */
  tasklet_schedule(&low0);
  tasklet_schedule(&low1);
  tasklet_schedule(&low2);
  tasklet_hi_schedule(&hi0);
  l4thread_sleep(1000);

  /* 3 lo : 3 hi */
  tasklet_schedule(&low0);
  tasklet_schedule(&low1);
  tasklet_schedule(&low2);
  tasklet_hi_schedule(&hi0);
  tasklet_hi_schedule(&hi1);
  tasklet_hi_schedule(&hi2);
  l4thread_sleep(1000);

  /* 1 lo : 1 hi */
  tasklet_schedule(&low0);
  tasklet_hi_schedule(&hi0);
  l4thread_sleep(10);

  /* 2 lo : 2 hi */
  tasklet_schedule(&low1);
  tasklet_hi_schedule(&hi1);
  tasklet_schedule(&low0);
  tasklet_hi_schedule(&hi0);
  l4thread_sleep(10);

  /* 1 hi (several times) */
  tasklet_hi_schedule(&hi2);
  tasklet_hi_schedule(&hi2);
  tasklet_hi_schedule(&hi2);

  l4thread_sleep(1000);

  LOG("DEBUG: End of debug_softirq");
}

/*****************************************************************************/
/** keventd */
/*****************************************************************************/
static int keventd_flag = 0;

static void keventd_func(void *p)
{
  LOGL("p=%p", p);
}

static void debug_keventd(void)
{
  int err;

  if ((err=l4dde_keventd_init()))
    {
      LOG_Error("initializing keventd (%d)", err);
      return;
    }

  struct tq_struct tq_test;

  INIT_TQUEUE(&tq_test, keventd_func, (void *)0x4711);

  schedule_task(&tq_test);
  l4thread_sleep(1000);

  schedule_task(&tq_test);
  l4thread_sleep(1000);

  schedule_task(&tq_test);
  l4thread_sleep(1000);
}

/*****************************************************************************/
/**    main    */
/*****************************************************************************/
static void usage(void)
{
  LOG(
"usage: dde_debug [OPTION]...\n"
"Debug dde_linux library functions. (Default is only library initialization.)\n"
"\n"
"  --irq[=n]            debug irq 0 at Omega0 by default\n"
"                       if n is set, use irq n at Omega0\n"
"  --jiffies[=n]        debug/measure 10 (or n) jiffies\n"
"  --lock[=n]           debug 5 (or n) spinlocks\n"
"  --malloc             debug memory allocations\n"
"  --pcidevs            show all PCI devices\n"
"  --pcimod             debug PCI module handling\n"
"  --softirq            debug softirq handling\n"
"  --keventd            debug keventd\n"
"  --timer              debug timers\n"
"\n"
"  --help               display this help (Doesn't exit immediately!)"
);
}

static void do_args(int argc, char *argv[])
{
  char c;

  static int long_check;
  static int long_optind;
  static struct option long_options[] = {
    {"jiffies", optional_argument, &long_check, 1},
    {"irq", optional_argument, &long_check, 2},
    {"pcidevs", no_argument, &long_check, 3},
    {"pcimod", no_argument, &long_check, 4},
    {"lock", optional_argument, &long_check, 5},
    {"timer", no_argument, &long_check, 6},
    {"softirq", no_argument, &long_check, 7},
    {"malloc", no_argument, &long_check, 8},
    {"keventd", no_argument, &long_check, 9},
    {"help", no_argument, &long_check, 99},
    {0, 0, 0, 0}
  };

/*      argc--; argv++;  skip program name */

  /* read command line arguments */
  optind = 0;
  long_optind = 0;
  while (1)
    {
      c = getopt_long_only(argc, argv, "", long_options, &long_optind);

      if (c == -1)
        break;

      switch (c)
        {
        case 0: /* long option */
          switch (long_check)
            {
            case 1: /* debug jiffies */
              jiffies_flag = 1;
              if (optarg)
                jiffies_num = atol(optarg);
              break;
            case 2: /* debug irqs */
              irq_flag = 1;
              if (optarg)
                irq_num = atol(optarg);
              break;
            case 3: /* debug pci devs */
              pci_devs_flag = 1;
              break;
            case 4: /* debug pci module init */
              pci_mod_flag = 1;
              break;
            case 5: /* debug spinlocks */
              lock_flag = 1;
              if (optarg)
                lock_num = atol(optarg);
              break;
            case 6: /* debug timers */
              timer_flag = 1;
              break;
            case 7: /* debug softirq */
              softirq_flag = 1;
              break;
            case 8: /* debug memory allocations */
              malloc_flag = 1;
              break;
            case 9: /* debug keventd */
              keventd_flag = 1;
              break;
            case 99: /* print usage */
              usage();
              break;
            default:
              /* ignore unknown */
              break;
            }
          break;
        default:
          /* ignore unknown */
          break;
        }
    }
}

int main(int argc, char *argv[])
{
  int err;
  l4io_info_t *io_info_addr = NULL;

  strcpy(&LOG_tag[0], FINAL_TAG);

  do_args(argc, argv);

  l4io_init(&io_info_addr, L4IO_DRV_INVALID);

  if ((err=l4dde_mm_init(128*1024, 128*1024)))
    {
      LOG_Error("initializing mm (%d)", err);
      exit(-1);
    }
  if ((err=l4dde_process_init()))
    {
      LOG_Error("initializing process (%d)", err);
      exit(-1);
    }

  LOG("DEBUG: \n"
      "irq=%c (%d)  jiffies=%c (%d)  lock=%c (%d)  pcidevs=%c  pcimod=%c\n"
      "malloc=%c  timers=%c  softirqs=%c keventd=%c\n",
      irq_flag ? 'y' : 'n', irq_num,
      jiffies_flag ? 'y' : 'n', jiffies_num,
      lock_flag ? 'y' : 'n', lock_num,
      pci_devs_flag ? 'y' : 'n', pci_mod_flag ? 'y' : 'n',
      malloc_flag ? 'y' : 'n', timer_flag ? 'y' : 'n',
      softirq_flag ? 'y' : 'n', keventd_flag ? 'y' : 'n');

  if (malloc_flag) debug_malloc();
  if (pci_devs_flag) debug_all_pci_devs();
  if (pci_mod_flag) debug_pci_module_init();
  if (irq_flag) debug_irq();
  if (jiffies_flag) debug_jiffies();
  if (lock_flag) debug_spinlock();
  if (timer_flag) debug_timers();
  if (softirq_flag) debug_softirq();
  if (keventd_flag) debug_keventd();

  if (err) exit(-1);

  exit(0);
}
