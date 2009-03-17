#include <l4/util/l4_macros.h>
#include <l4/util/macros.h>
#include <l4/util/util.h>
#include <l4/l4rm/l4rm.h>
#include <l4/log/l4log.h>

#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

static const int SZ = 10;

static void show_region_list(char *str)
{
	LOG_printf("==================================================================\n");
	LOG_printf("%s\n", str);
	l4rm_show_region_list();
	LOG_printf("==================================================================\n");
}


int main(void)
{
	char *ptr = 0;
	int fd;
	int i;

	l4_sleep(2000);

	LOG("Opening /data/foo_txt");
	fd = open("/data/foo_txt", O_RDONLY);
	if (fd < 0)
	{
		LOG_printf("open failed: %d %d\n", fd, errno);
		return -1;
	}

	show_region_list("before mmap");

	ptr = mmap(NULL,
	           SZ,
	           PROT_READ,
	           MAP_PRIVATE | MAP_FIXED,
	           fd, 0);

	if (ptr == MAP_FAILED)
	{
		LOG_printf("mmap failed\n");
		return -1;
	}
	LOG("ptr = %p", ptr);

	show_region_list("after mmap");

	LOG("printing %d chars...", SZ);
	LOG_printf("'");
	for (i=0; i < SZ; i++) {
		LOG_printf("%c", *(ptr+i));
	}
	LOG_printf("'\n");

	i = munmap(ptr, SZ);
	LOG("munmap: %d", i);

	show_region_list("after munmap");

	return 0;
}
