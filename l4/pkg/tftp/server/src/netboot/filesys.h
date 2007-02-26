
#include "pc_slice.h"

int tftp_mount (void);
int tftp_read (char *buf, int len);
int tftp_dir (char *dirname);
void tftp_close (void);

#define NUM_FSYS	1

struct fsys_entry
{
  char *name;
  int (*mount_func) (void);
  int (*read_func) (char *buf, int len);
  int (*dir_func) (char *dirname);
  void (*close_func) (void);
  int (*embed_func) (int *start_sector, int needed_sectors);
};

extern int print_possibilities;
extern int fsmax;
extern struct fsys_entry fsys_table[NUM_FSYS + 1];
