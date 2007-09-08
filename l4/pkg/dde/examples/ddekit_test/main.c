#include <l4/dde/dde.h>
#include <l4/dde/ddekit/thread.h>
#include <l4/dde/ddekit/printf.h>
#include <l4/dde/ddekit/assert.h>

#include <l4/util/util.h>
#include <l4/generic_io/libio.h>

#define VERBOSE 1


/****************
 ** Timer test **
 ****************/

#include <l4/dde/ddekit/timer.h>

extern volatile unsigned long long jiffies;
extern unsigned long HZ;

static int twenty_timer = -1;

static void __attribute__((unused)) dummy(void *arg) { }

static void timer_func(void *arg)
{
	ddekit_printf("Timed func called with arg %d\n", (int)arg);

	if ((int)arg == 15) {
		int del = ddekit_del_timer(twenty_timer);
		ddekit_printf("\tdropped 20s timer: %d\n", del);
	}

	if ((int)arg == 30)
		ddekit_printf("\033[32;1mEND TIMER TEST\033[0m\n");
}


/* One tick each second. */
static void tick(void *arg)
{
	static int cnt=0;
	int r=0;

	ddekit_printf("tick %d\n", cnt++);
	
	r = ddekit_add_timer(tick, 0, jiffies + HZ);
}


static void timer_test(void)
{
	int i;

	ddekit_printf("\033[32;1mBEGIN TIMER TEST\033[0m\n");

	ddekit_printf("HZ: %d\n", HZ);
	ddekit_printf("JIFFIES: %lld\n", jiffies);
	
	ddekit_init_timers();
	ddekit_thread_msleep(1000);

	i = ddekit_add_timer(tick, 0, jiffies+HZ);
	ddekit_printf("added timer: %d\n", i);

	i = ddekit_add_timer(timer_func, (void *)5, jiffies + 5*HZ);
	ddekit_printf("added timer: %d\n", i);
	i = ddekit_add_timer(timer_func, (void *)10, jiffies + 10*HZ);
	ddekit_printf("added timer: %d\n", i);
	i = ddekit_add_timer(timer_func, (void *)15, jiffies + 15*HZ);
	ddekit_printf("added timer: %d\n", i);
	twenty_timer = ddekit_add_timer(timer_func, (void *)20, jiffies + 20*HZ);
	ddekit_printf("added timer: %d\n", twenty_timer);
	i = ddekit_add_timer(timer_func, (void *)30, jiffies + 30*HZ);
	ddekit_printf("added timer: %d\n", i);
}

/********************************
 ** Memory and page-table test **
 ********************************/

#include <l4/dde/ddekit/memory.h>
#include <l4/dde/ddekit/pgtab.h>

static void memory_test(void)
{
	{
		/* simple malloc */
		ddekit_printf("\033[32;1mBEGIN SIMPLE MEMORY TEST\033[0m\n");

		int i, j;
		unsigned size = 508;
		unsigned *p[10];

		for (i = 0; i < 10; ++i) {
			p[i] = ddekit_simple_malloc(size);
			for (j = 0; j < size/sizeof(unsigned); ++j)
				(p[i])[j] = i;
			ddekit_printf("malloc(%d) => %p\n", size, p[i]);
		}

		i = 4;
		ddekit_simple_free(p[i]);
		ddekit_printf("free(%p)\n", p[i]);

		p[i] = ddekit_simple_malloc(size);
		for (j = 0; j < size/sizeof(unsigned); ++j)
			(p[i])[j] = 0xeeeeeeee;
		ddekit_printf("malloc(%d) => %p\n", size, p[i]);

		for (i = 9; i >= 0; --i) {
			ddekit_simple_free(p[i]);
			ddekit_printf("free(%p)\n", p[i]);
		}

		i = 0;
		p[i] = ddekit_simple_malloc(size);
		for (j = 0; j < size/sizeof(unsigned); ++j)
			(p[i])[j] = 0xaaaaaaaa;
		ddekit_printf("malloc(%d) => %p\n", size, p[i]);

		ddekit_printf("\033[32;1mEND SIMPLE MEMORY TEST\033[0m\n");
	}
	{
		/* large malloc */
		ddekit_printf("\033[32;1mBEGIN LARGE MEMORY TEST\033[0m\n");

		const l4_size_t malloc_size = 1024 * 4096;

		char *p = ddekit_large_malloc(malloc_size);

		ddekit_printf("virt->phys mappings:\n");
		char *tmp = p;
		ddekit_printf("  %p -> %p\n", tmp, ddekit_pgtab_get_physaddr(tmp));
		tmp = p + malloc_size / 2;
		ddekit_printf("  %p -> %p\n", tmp, ddekit_pgtab_get_physaddr(tmp));
		tmp = p + malloc_size;
		ddekit_printf("Expected error follows...\n");
		ddekit_printf("  %p -> %p\n", tmp, ddekit_pgtab_get_physaddr(tmp));

		ddekit_large_free(p);
		tmp = p + malloc_size;
		ddekit_printf("Expected error follows...\n");
		ddekit_printf("  %p -> %p\n", tmp, ddekit_pgtab_get_physaddr(tmp));

		p = ddekit_large_malloc(malloc_size);

		ddekit_printf("virt->phys mappings:\n");
		tmp = p;
		ddekit_printf("  %p -> %p\n", tmp, ddekit_pgtab_get_physaddr(tmp));
		tmp = p + malloc_size / 2;
		ddekit_printf("  %p -> %p\n", tmp, ddekit_pgtab_get_physaddr(tmp));
		tmp = p + malloc_size;
		ddekit_printf("Expected error follows...\n");
		ddekit_printf("  %p -> %p\n", tmp, ddekit_pgtab_get_physaddr(tmp));

		ddekit_large_free(p);

		ddekit_printf("\033[32;1mEND LARGE MEMORY TEST\033[0m\n");
	}
	{
		int x = 0;
		/* slabs */
		ddekit_printf("\033[32;1mBEGIN SLAB MEMORY TEST\033[0m\n");

		/* Test slabs for contiguous and non-contiguous case. */
		for ( ; x < 2; ++x) {
			ddekit_printf("Contiguous mode set to %d\n", x);

			struct ddekit_slab *slab = ddekit_slab_init(256, x);
			if (!slab) ddekit_debug("slab cache init failed");

			unsigned const magic = 0xdeadbeef;
			ddekit_slab_set_data(slab, (void *)magic);
			if (ddekit_slab_get_data(slab) != (void *)magic)
				ddekit_debug("user pointer differs");

			unsigned i;
			unsigned *p[64];

			for (i = 0; i < sizeof(p)/sizeof(*p); ++i) {
				p[i]  = ddekit_slab_alloc(slab);
				*p[i] = i | 0xaffe0000;
				if (i % 8 == 0) {
					if (x)
						ddekit_printf("slab_alloc[%d] => %p (%p phys)\n",
						              i, p[i], ddekit_pgtab_get_physaddr(p[i]));
					else
						ddekit_printf("slab_alloc[%d] => %p (no phys)\n", i, p[i]);
				}
			}

			for (i = 0; i < sizeof(p)/sizeof(*p); ++i) {
				ddekit_slab_free(slab, p[i]);
			}
		}

		ddekit_printf("\033[32;1mEND SLAB MEMORY TEST\033[0m\n");
	}
}


int main(int argc, char **argv)
{
	ddekit_log(1, "Hey there, %s running...", argv[0]);

	ddekit_init();

	if (1) memory_test();
	if (1) timer_test();

	l4_sleep_forever();

	return 0;
}
