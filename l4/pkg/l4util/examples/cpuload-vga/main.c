#include <l4/util/util.h>
#include <l4/util/rdtsc.h>

#include <stdio.h>
#include <string.h>

static inline void print_vga(int x, int y, const char *s)
{
  unsigned char *b = (unsigned char *)0xb8000 + y * 160 + x * 2;
  while (*s)
    {
      *b = *s;
      b += 2;
      s++;
    }
}

int main(void)
{
  l4_uint32_t tsc = 0, pmc = 0;

  while (1)
    {
      l4_uint32_t new_tsc = l4_rdtsc_32(), new_pmc = l4_rdpmc_32(0);

      if (tsc && pmc)
	{
	  char load_str[16];
	  l4_uint32_t load = (new_pmc - pmc) / ((new_tsc - tsc) / 1000);
	  if (load > 1000)
	    strcpy(load_str, "---.-%");
	  else
	    sprintf(load_str, "%3d.%d%%", load / 10, load % 10);
	  print_vga(70, 0, load_str);
	}

      tsc = new_tsc;
      pmc = new_pmc;

      l4_sleep(400);
    }

  return 0;
}
