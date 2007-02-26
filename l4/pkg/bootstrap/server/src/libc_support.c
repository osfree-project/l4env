#define _GNU_SOURCE
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <sys/types.h>

#include "base_critical.h"
#ifdef ARCH_x86
#include "ARCH-x86/serial.h"
#endif
#ifdef ARCH_arm
#include <l4/arm_drivers_c/uart.h>
#include <l4/arm_drivers_c/hw.h>
#include <l4/crtx/crt0.h>
#endif

int have_hercules(void);

int
have_hercules(void)
{
  return 0;
}

/* for the l4_start_stop dietlibc backend */
#ifdef ARCH_x86
void crt0_sys_destruction(void);
void crt0_sys_destruction(void)
{
}
#endif

#ifdef ARCH_arm
extern char _bss_start[], _bss_end[];
extern void startup(void);

void __main(void)
{
  hw_init();
  memset(_bss_start, 0, (char *)&crt0_stack_low - _bss_start);
  memset((char *)&crt0_stack_high, 0, _bss_end - (char *)&crt0_stack_high);
  crt0_construction();
  uart_startup(uart_base(), 0);
  uart_change_mode(uart_get_mode(UART_MODE_TYPE_8N1), 115200);
  startup();
  while(1);
}
#endif

void reboot(void);
void reboot(void)
{
  extern void l4util_reboot_arch(void);
  l4util_reboot_arch();
}

#ifdef ARCH_x86
static void direct_cons_putchar(unsigned char c)
{
  static int ofs  = -1;
  static int esc;
  static int esc_val;
  static int attr = 0x07;
  unsigned char *vidbase = (unsigned char*)0xb8000;

  base_critical_enter();

  if (ofs < 0)
    {
      /* Called for the first time - initialize.  */
      ofs = 80*2*24;
      direct_cons_putchar('\n');
    }

  switch (esc)
    {
    case 1:
      if (c == '[')
	{
	  esc++;
	  goto done;
	}
      esc = 0;
      break;

    case 2:
      if (c >= '0' && c <= '9')
	{
	  esc_val = 10*esc_val + c - '0';
	  goto done;
	}
      if (c == 'm')
	{
	  attr = esc_val ? 0x0f : 0x07;
	  goto done;
	}
      esc = 0;
      break;
    }

  switch (c)
    {
    case '\n':
      bcopy(vidbase+80*2, vidbase, 80*2*24);
      bzero(vidbase+80*2*24, 80*2);
      /* fall through... */
    case '\r':
      ofs = 0;
      break;

    case '\t':
      ofs = (ofs + 8) & ~7;
      break;

    case '\033':
      esc = 1;
      esc_val = 0;
      break;

    default:
      /* Wrap if we reach the end of a line.  */
      if (ofs >= 80)
	direct_cons_putchar('\n');

      /* Stuff the character into the video buffer. */
	{
	  volatile unsigned char *p = vidbase + 80*2*24 + ofs*2;
	  p[0] = c;
	  p[1] = attr;
	  ofs++;
	}
      break;
    }

done:
  base_critical_leave();
}
#endif /* ARCH_x86 */

static inline void direct_outnstring(const char *buf, int count)
{
#ifdef ARCH_x86
  while (count--)
    {
      direct_cons_putchar(*buf);
      com_cons_putchar(*buf++);
    }
#endif
#ifdef ARCH_arm
  uart_write(buf, count);
#endif
}


int write(int fd, const void *buf, size_t count) __THROW;
int write(int fd, const void *buf, size_t count) __THROW
{
  // just accept write to stdout and stderr
  if (fd == STDOUT_FILENO || fd == STDERR_FILENO)
    {
      direct_outnstring((const char*)buf, count);
      return count;
    }

  // writes to other fds shall fail fast
  errno = EBADF;
  return -1;
}

#ifdef USE_DIETLIBC
off_t lseek(int fd, off_t offset, int whence) __THROW;
off_t lseek(int fd, off_t offset, int whence) __THROW
#else
off64_t lseek64(int fd, off64_t offset, int whence) __THROW;
off64_t lseek64(int fd, off64_t offset, int whence) __THROW
#endif
{
  // just accept lseek to stdin, stdout and stderr
  if ((fd != STDIN_FILENO) &&
      (fd != STDOUT_FILENO) &&
      (fd != STDERR_FILENO))
    {
      errno = EBADF;
      return -1;
    }

  switch (whence)
    {
    case SEEK_SET:
      if (offset < 0)
	{
	  errno = EINVAL;
	  return -1;
	}
      return offset;
    case SEEK_CUR:
    case SEEK_END:
      return 0;
    default:
      errno = EINVAL;
      return -1;
    }
}
