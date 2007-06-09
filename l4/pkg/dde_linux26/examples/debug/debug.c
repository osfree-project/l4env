/* $Id$ */
/*****************************************************************************/
/**
 * \file	dde_linux26/examples/debug/debug.c
 *
 * \brief	DDE Example (debugging purposes)
 *
 * \author	Marek Menzer <mm19@os.inf.tu-dresden.de>
 *
 * Based on dde_linux version by Christian Helmuth <ch12@os.inf.tu-dresden.de>
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
#include <linux/kobject.h>
#include <linux/kobj_map.h>
#include <asm/delay.h>

/* OSKit */
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
/** memory */
/*****************************************************************************/
static int malloc_flag = 0;

static void debug_malloc(void)
{
  void *p0, *p1, *p2;

  /* simple malloc/free pairs */
  LOG("Allocating ...");
  LOG("(1) %d bytes kmem available", l4dde_mm_kmem_avail());
  p0 = kmalloc(1024, GFP_KERNEL); Assert(p0);
  kfree(p0);
  p0 = kmalloc(128, GFP_KERNEL); Assert(p0);
  p1 = kmalloc(0x2000, GFP_KERNEL); Assert(p1);
  LOG("(2) %d bytes kmem available", l4dde_mm_kmem_avail());
  kfree(p1);
  kfree(p0);
  p0 = kmalloc(128, GFP_KERNEL); Assert(p0);
  p1 = kmalloc(0x2000, GFP_KERNEL); Assert(p1);
  p2 = kmalloc(100 * 1024, GFP_KERNEL); Assert(p2);
  LOG("(3) %d bytes kmem available", l4dde_mm_kmem_avail());
  kfree(p2);
  kfree(p1);
  kfree(p0);
  LOG("(4) %d bytes kmem available", l4dde_mm_kmem_avail());

  /* alloc too much */
  LOG("Trigger Out of Memory ...");
  p0 = kmalloc(1024*1024, GFP_KERNEL); Assert(p0);
  LOG("(5) %d bytes kmem available", l4dde_mm_kmem_avail());
}

/*****************************************************************************/
/** IRQ */
/*****************************************************************************/
static int free_me = 0;
static int runs = 2;
static irqreturn_t debug_irq_handler(int irq, void *id, struct pt_regs *pt)
{
  static int count = 100;
  static unsigned long old = 0;

  if (!in_irq())
    Panic("in_irq() not set in interrupt handler");

  if (!in_softirq())
    Panic("in_softirq() not set in interrupt handler");

  if (!runs)
    free_me = 1;

  /* every 100 IRQs */
  if (!count)
    {
      LOG("-> 100 (%ld)", jiffies - old);
      count = 100;
      old = jiffies;
      runs--;
    }
  count--;
  return 0;
}

static int irq_flag = 0;
static int irq_num = 0;
static void debug_irq(void)
{
  int err;

  /* initialize irq module of dde_linux 
     num < 16 RMGR
     num > 15 Omega0 */
  if ((err=l4dde_irq_init()))
    {
      LOG_Error("initializing irqs (%d)", err);
      return;
    }

  err = request_irq(irq_num & 0x0f, debug_irq_handler, 0, "haha", 0);
  if (err)
    LOG("request_irq failed (%d)", err);

  while (!free_me)
    l4thread_sleep(100);

  free_irq(irq_num & 0x0f, 0);

  LOG("requesting irq the 2nd time\n");
  runs = 2;
  free_me = 0;

  err = request_irq(irq_num & 0x0f, debug_irq_handler, 0, "haha", 0);
  if (err)
    LOG("request_irq failed (%d)", err);

  while (!free_me)
    l4thread_sleep(100);

  free_irq(irq_num & 0x0f, 0);
}

/*****************************************************************************/
/** jiffies */
/*****************************************************************************/
static int jiffies_flag = 0;
static int jiffies_num = 10;
static void debug_jiffies(void)
{
  LOG("HZ = %lu (%p)", HZ, &HZ);

  while (jiffies_num)
    {
      LOG("%lu jiffies (%p) since startup", jiffies, &jiffies);
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
        Panic("locked spinlock is NOT locked");

      t0 = jiffies;
      l4thread_sleep(5 * sleep_time);
      t1 = jiffies;

      spin_unlock_irqrestore(&grabme, flags);

      LOG("  %6s| from %5ld\n"
          "  %6s|   to %5ld (%ld jiffies)",
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
    Panic("in_interrupt() not set in timer handler");

  if (!i)
    timers_expired++;
  if (i == TIMER_COOKIE)
    /* bug triggered */
    LOG("Ouch, found cookie data (0x%lx) @ %ld jiffies", i, jiffies);
  else
    /* normal processing */
    LOG("timer_func [%ld] @ %ld jiffies", i, jiffies);
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

  LOG("expiration @ (%ld, %ld, %ld, %ld)",
      t0.expires, t1.expires, t2.expires, t3.expires);

  add_timer(&t1);
  add_timer(&t2);
  add_timer(&t0);
  add_timer(&t3);

  LOG("waiting for timer expiration ...");

  while (!timers_expired)
    l4thread_sleep(100);	/* wait for latest timer expiration as timers
				   are on stack !!! */

  /* now trigger bug #1 in old dde version:
   * remove last timer while we're waiting for it
   */
  LOG("\ntrigger known bug #1 in old version");

  timers_expired = 0;

  init_timer(&t0);
  t0.function = __timer_func;

  t0.expires = jiffies + 500;
  t0.data = 0UL;

  LOG("expiration @ (%ld)", t0.expires);

  add_timer(&t0);

  l4thread_sleep(100);

  del_timer(&t0);

  /* now t0 is invalid, but time.c still used it ... */

  t0.expires = 0;		/* be sure it will be processed */
  t0.data = TIMER_COOKIE;	/* place a cookie */
  t0.entry.next = &t1.entry;
  t0.entry.prev = &t1.entry;	/* now the "syntetic" _list entry_ is valid again */

  LOG("waiting for timer expiration ...");

  l4thread_sleep(6000);

  /* now trigger bug #2 in old dde version:
   * timer expired before examination
   */
  LOG("\ntrigger known bug #2 in old version");

  timers_expired = 0;

  init_timer(&t0);
  t0.function = __timer_func;

  t0.expires = jiffies - 2;
  t0.data = 0UL;

  LOG("expiration @ (%ld)", t0.expires);

  add_timer(&t0);

  while (!timers_expired)
    l4thread_sleep(100);	/* wait for latest timer expiration as timers
				   are on stack !!! */
}

/*****************************************************************************/
/** softirqs */
/*****************************************************************************/
static int softirq_flag = 0;

static void __softirq_func(unsigned long i)
{
  if (!in_softirq())
    Panic("in_softirq() not set in softirq handler");

  if (!in_irq())
    Panic("in_irq() set in softirq handler");

  LOG("softirq [%ld] @ %ld jiffies (lthread %0x)",
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

  LOG("End of debug_softirq");
}

/*****************************************************************************/
/** Kobject declarations */
/*****************************************************************************/
static struct kobj_type ktype_debug = {
};

static int debug_hotplug_filter(struct kset *kset, struct kobject *kobj)
{
    struct kobj_type *ktype = get_ktype(kobj);
    
    return (ktype == &ktype_debug);
}

static struct kset_hotplug_ops debug_hotplug_ops = {
    .filter = debug_hotplug_filter,
};

static decl_subsys(debug, &ktype_debug, &debug_hotplug_ops);

/*****************************************************************************/
/** kobj_map */
/*****************************************************************************/
static int kobj_map_flag = 0;
static struct kobj_map *debug_map;

static struct kobject *base_probe(dev_t dev, int *index, void *data)
{
    LOG("probe was asked for Dev %i:%i with index %i and data '%s'", MAJOR(dev), MINOR(dev), *index,(char*)data);
    return NULL;
}

static int debug_lock(dev_t dev, void *data)
{
    return 0;
}

static void debug_kobj_map(void)
{
  int index, err;
  
  debug_map = kobj_map_init(base_probe, &debug_subsys);
  if (debug_map == NULL)
  {
    LOG("Couldn't init kobj_map");
    goto error_out;
  }
  subsystem_register(&debug_subsys);
  
  kobj_lookup(debug_map, MKDEV(4,1), &index);
  kobj_lookup(debug_map, MKDEV(4,2), &index);
  kobj_lookup(debug_map, MKDEV(4,7), &index);
  kobj_lookup(debug_map, MKDEV(4,11), &index);
  kobj_lookup(debug_map, MKDEV(4,12), &index);
  LOG(" ");

  LOG("Mapping 'probe 1' function from Major %i Minor %i to Minor %i",4,2,11);
  err = kobj_map(debug_map, MKDEV(4,2), 10, NULL, base_probe, debug_lock, "probe 1");
  if (err)
  {
    LOG("Error (%i) mapping devices",err);
    goto error_out;
  }

  LOG("Mapping 'probe 2' function from Major %i Minor %i to Minor %i",4,6,7);
  err = kobj_map(debug_map, MKDEV(4,6), 2, NULL, base_probe, debug_lock, "probe 2");
  if (err)
  {
    LOG("Error (%i) mapping devices",err);
    goto error_out;
  }

  kobj_lookup(debug_map, MKDEV(4,1), &index);
  kobj_lookup(debug_map, MKDEV(4,2), &index);
  kobj_lookup(debug_map, MKDEV(4,7), &index);
  kobj_lookup(debug_map, MKDEV(4,11), &index);
  kobj_lookup(debug_map, MKDEV(4,12), &index);
  LOG(" ");
  
  LOG("Unmapping 'probe 1' function");
  kobj_unmap(debug_map, MKDEV(4,2), 10);

  kobj_lookup(debug_map, MKDEV(4,1), &index);
  kobj_lookup(debug_map, MKDEV(4,2), &index);
  kobj_lookup(debug_map, MKDEV(4,7), &index);
  kobj_lookup(debug_map, MKDEV(4,11), &index);
  kobj_lookup(debug_map, MKDEV(4,12), &index);
  LOG(" ");

error_out:
  LOG("End of debug_kobj_map");
}

/*****************************************************************************/
/** kobjects */
/*****************************************************************************/
static int kobjects_flag = 0;
struct debug_object {
    int data;
    char *descr;
    struct kobject kobj;
};

static void debug_kobjects(void)
{
  int err;
  struct debug_object *dobj;
  struct kobject *k;
  
  err = subsystem_register(&debug_subsys);
  if (err)
  {
     LOG("Error (%i) registering subsystem",err);
     goto error_out;
  }

  dobj = kmalloc(sizeof(struct debug_object), GFP_KERNEL);
  memset(dobj, 0, sizeof(struct debug_object));
  dobj->data = 1;
  dobj->descr = "some text";
  kobj_set_kset_s(dobj,debug_subsys);
  kobject_init(&dobj->kobj);
  kobject_set_name(&dobj->kobj,"Kobject 1");
  kobject_add(&dobj->kobj);

  k = kset_find_obj(&debug_subsys.kset,"Kobject 1");
  if (k == NULL)
  {
     LOG("Kobject not found");
     goto error_out;
  }
  dobj = container_of(k, struct debug_object, kobj);
  LOG("Kobject '%s' found with description '%s'",dobj->kobj.name,dobj->descr);
  
error_out:
  LOG("End of debug_kobjects");
}

/*****************************************************************************/
/** workqueues */
/*****************************************************************************/
static int workqueue_flag = 0;

static void work_function(void *data)
{
    LOG("Completed work '%s'",(char*)data);
}

static void debug_workqueues(void)
{
    static struct work_struct work1, work2, work3;
    static int delay;
    
    l4dde_time_init();
    l4dde_softirq_init();
    l4dde_process_init();
    l4dde_workqueues_init();
    
    INIT_WORK(&work1,work_function,"Work 1");
    delay = 800;
    LOG("Scheduling delayed work '%s' for completion in %i seconds",(char*)work1.data,delay/100);
    schedule_delayed_work(&work1, delay);

    INIT_WORK(&work2,work_function,"Work 2");
    delay = 600;
    LOG("Scheduling delayed work '%s' for completion in %i seconds",(char*)work2.data,delay/100);
    schedule_delayed_work(&work2, delay);

    INIT_WORK(&work3,work_function,"Work 3");
    LOG("Scheduling immediate work '%s'",(char*)work3.data);
    schedule_work(&work3);

    LOG("Waiting for work completion");
    flush_scheduled_work();
    udelay(10000000); // wait 10 seconds
    
    LOG("End of debug_workqueues");
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
"  --irq[=n]		debug irq 0 at RMGR by default\n"
"			if n is set and n<16 use irq n at RMGR\n"
"			if n is set and n>15 use irq (n&0x0f) at Omega0\n"
"  --jiffies[=n]	debug/measure 10 (or n) jiffies\n"
"  --lock[=n]		debug 5 (or n) spinlocks\n"
"  --malloc		debug memory allocations\n"
"  --pcidevs		show all PCI devices\n"
"  --softirq		debug softirq handling\n"
"  --timer		debug timers\n"
"  --kobjmap		debug kobj mappings\n"
"  --kobjects		debug kobjects\n"
"  --workqueues		debug workqueues\n"
"\n"
"  --help		display this help (Doesn't exit immediately!)\n"
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
    {"lock", optional_argument, &long_check, 5},
    {"timer", no_argument, &long_check, 6},
    {"softirq", no_argument, &long_check, 7},
    {"malloc", no_argument, &long_check, 8},
    {"kobjmap", no_argument, &long_check, 9},
    {"kobjects", no_argument, &long_check, 10},
    {"workqueues", no_argument, &long_check, 11},
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
	case 0:		/* long option */
	  switch (long_check)
	    {
	    case 1:		/* debug jiffies */
	      jiffies_flag = 1;
	      if (optarg)
		jiffies_num = atol(optarg);
	      break;
	    case 2:		/* debug irqs */
	      irq_flag = 1;
	      if (optarg)
		irq_num = atol(optarg);
	      break;
	    case 3:		/* debug pci devs */
	      pci_devs_flag = 1;
	      break;
	    case 5:		/* debug spinlocks */
	      lock_flag = 1;
	      if (optarg)
		lock_num = atol(optarg);
	      break;
	    case 6:		/* debug timers */
	      timer_flag = 1;
	      break;
	    case 7:		/* debug softirq */
	      softirq_flag = 1;
	      break;
	    case 8:		/* debug memory allocations */
	      malloc_flag = 1;
	      break;
	    case 9:		/* debug kobj mappings */
	      kobj_map_flag = 1;
	      break;
	    case 10:		/* debug kobjects */
	      kobjects_flag = 1;
	      break;
	    case 11:		/* debug workqueues */
	      workqueue_flag = 1;
	      break;
	    case 99:		/* print usage */
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

  LOG("DEBUG: \n"
      "irq=%c (%d)  jiffies=%c (%d)  lock=%c (%d)  pcidevs=%c\n"
      "malloc=%c  timers=%c  softirqs=%c  kobjmap=%c kobjects=%c workqueues=%c\n",
      irq_flag ? 'y' : 'n', irq_num,
      jiffies_flag ? 'y' : 'n', jiffies_num,
      lock_flag ? 'y' : 'n', lock_num,
      pci_devs_flag ? 'y' : 'n',
      malloc_flag ? 'y' : 'n', timer_flag ? 'y' : 'n',
      softirq_flag ? 'y' : 'n', kobj_map_flag ? 'y' : 'n',
      kobjects_flag ? 'y' : 'n', workqueue_flag ? 'y' : 'n');

  if (malloc_flag) debug_malloc();
  if (pci_devs_flag) debug_all_pci_devs();
  if (irq_flag) debug_irq();
  if (jiffies_flag) debug_jiffies();
  if (lock_flag) debug_spinlock();
  if (timer_flag) debug_timers();
  if (softirq_flag) debug_softirq();
  if (kobj_map_flag) debug_kobj_map();
  if (kobjects_flag) debug_kobjects();
  if (workqueue_flag) debug_workqueues();

  if (err) goto err;

  exit(0);
 err:
  exit(-1);
}
