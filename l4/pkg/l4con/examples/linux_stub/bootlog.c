/*
 * bootlog.c
 *
 * Buffering of initial output
 */

#include <linux/types.h>
#include <linux/tty.h>
#include <linux/console.h>
#include <linux/version.h>
#include <l4/sys/kdebug.h>

#define DROPSCON_BOOTLOG_BUF_LEN	(16384) /* must be a power of 2 */
#define DROPSCON_BOOTLOG_BUF_MASK	(DROPSCON_BOOTLOG_BUF_LEN-1)

static char dropscon_bootlog_buf[DROPSCON_BOOTLOG_BUF_LEN];
static unsigned dropscon_bootlog_start = 0;
static unsigned dropscon_bootlog_tail  = 0;
static unsigned dropscon_bootlogged_chars = 0;

unsigned dropscon_bootlog_init_done = 0;

static void
dropscon_bootlog_write(struct console *c, const char *p, unsigned count)
{
  int i;

  for (i=0; i<count; i++, p++)
    {
      dropscon_bootlog_start &= DROPSCON_BOOTLOG_BUF_MASK;
      dropscon_bootlog_tail  &= DROPSCON_BOOTLOG_BUF_MASK;
      dropscon_bootlog_buf[dropscon_bootlog_tail++] = *p;
      if (dropscon_bootlogged_chars < DROPSCON_BOOTLOG_BUF_LEN)
	dropscon_bootlogged_chars++;
      else
	dropscon_bootlog_start++;
      if (*p == '\n')
	outchar('\r');
      outchar(*p);
    }
}

static struct console dropscon_bootlog_cons =
{
  name:     "drops",
  write:    dropscon_bootlog_write,
  read:     NULL,
  device:   NULL,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,19)
  wait_key: NULL,
#endif
  unblank:  NULL,
  setup:    NULL,
  flags:    CON_PRINTBUFFER | CON_ENABLED,
  index:    -1,
  cflag:    0,
  next:     NULL
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
extern void console_setup(char *str, int *ints);
#endif

void
dropscon_bootlog_init(void)
{
  dropscon_bootlog_start = 
  dropscon_bootlog_tail  = 
  dropscon_bootlogged_chars = 0;
  
  register_console(&dropscon_bootlog_cons);
  /* choose tty console to be the preferred console */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
  console_setup("tty", 0);
#endif
  
  dropscon_bootlog_init_done = 1;
}

void
dropscon_bootlog_done(void)
{
  unregister_console(&dropscon_bootlog_cons);
  dropscon_bootlog_init_done = 0;
}

char
dropscon_bootlog_read(void)
{
  if (dropscon_bootlogged_chars)
    {
      dropscon_bootlogged_chars--;
      return dropscon_bootlog_buf[(dropscon_bootlog_start++) &
				  DROPSCON_BOOTLOG_BUF_MASK];
    }
  
  return 0;
}

