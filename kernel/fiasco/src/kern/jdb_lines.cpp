INTERFACE:

class Jdb_lines
{
private:
  typedef struct
    {
      unsigned int   addr;
      unsigned short line;
    } __attribute__ ((packed)) line_t;
  
  typedef struct
    {
      line_t *str;
      unsigned beg;
      unsigned end;
    } task_lines_t;
  
  static task_lines_t task_lines[2048];
};


IMPLEMENTATION:

#include <cstdio>
#include <cstring>
#include "kmem.h"

Jdb_lines::task_lines_t Jdb_lines::task_lines[2048];

// determine if line is included in kernel lines
// by checking the address
static inline
bool
Jdb_lines::addr_in_kernel(unsigned addr)
{
  return (   (task_lines[0].str != 0)
          && (task_lines[0].beg <= addr));
}

// search line name that matches for a specific address
PUBLIC static
bool
Jdb_lines::match_address_to_line(unsigned address, unsigned task,
				 char *line, unsigned line_size,
				 int show_header_files)
{
  // we have no line numbers for kernel
  if (addr_in_kernel(address))
    task = 0;

  if (task>=2048)
    return false;

  const char *str = (const char*)task_lines[task].str;

  if (   (str != 0)
      && (task_lines[task].beg <= address)
      && (task_lines[task].end >= address))
    {
      int show_file = 1;
      line_t *lin;
      const char *dir="", *file="";
      
      for (lin = task_lines[task].str; lin->addr || lin->line; lin++)
	{
	  if (lin->line == 0xfffe)
	    {
	      // this is a directory entry
	      dir = str + lin->addr;
	    }
	  else if (lin->line == 0xffff)
	    {
	      // this is a file name entry
	      file = str + lin->addr;
	      show_file = 1;
	    }
	  else if (lin->line == 0xfffd)
	    {
	      // this is a header file name entry
	      file = str + lin->addr;
	      show_file = show_header_files;
	    }
	  else if ((lin->addr == address) && show_file)
	    {
	      // this is a line
	      sprint_line(dir, file, lin->line, line, line_size);
	      return true;
	    }
	}
    }

  return false;
}

// search line name that matches for a specific address
PUBLIC static
bool
Jdb_lines::match_address_to_line_fuzzy(unsigned address, unsigned task,
				       char *line, unsigned line_size,
				       int show_header_files)
{
  // we have no line numbers for kernel
  if (addr_in_kernel(address))
    task = 0;

  if (task>=2048)
    return false;

  const char *str = (const char*)task_lines[task].str;

  if (   (str != 0)
      && (task_lines[task].beg <= address)
      && (task_lines[task].end >= address))
    {
      int show_file = 1;
      unsigned best_address=0xffffffff, best_line=0;
      line_t *lin;
      const char *dir="", *file="", *best_dir=0, *best_file=0;
      
      for (lin = task_lines[task].str; lin->addr || lin->line; lin++)
	{
	  if (lin->line == 0xfffe)
	    {
	      // this is a directory entry
	      dir = str + lin->addr;
	    }
	  else if (lin->line == 0xffff)
	    {
	      // this is a file name entry
	      file = str + lin->addr;
	      show_file = 1;
	    }
	  else if (lin->line == 0xfffd)
	    {
	      // this is a header file name entry
	      file = str + lin->addr;
	      show_file = show_header_files;
	    }
	  else if ((lin->addr <= address)
		   && (address-lin->addr < best_address)
		   && show_file)
	    {
	      best_address = address-lin->addr;
	      best_dir     = dir;
	      best_file    = file;
	      best_line    = lin->line;
	    }
	}
      if (best_address < 0xffffffff)
	{
	  sprint_line(best_dir, best_file, best_line, line, line_size);
	  return true;
	}
    }

  return false;
}

// truncate string if its length exceeds str_size and create line
static
void
Jdb_lines::sprint_line(const char *dir, const char *fname, unsigned line,
		       char *str, unsigned str_size)
{
  unsigned d = strlen(dir);
  unsigned f = strlen(fname);

  if (fname[0]=='/')
    d=0;

  // handle fname in form of ../../../../foo/bar.h
  while (d>0 && f>2 && fname[0]=='.' && fname[1]=='.' && fname[2]=='/')
    {
      if (d>1)
	{
	  // cut last directory
	  while ((d>1) && (dir[d-2]!='/'))
	    d--;
	  d--;
	  // cut first ../ directory of fname
	  fname+=3;
	  f-=3;
	}
    }

  str_size -= 6;		// for line number
  
  if (str_size < 10)
    {
      *str = '\0';		// sanity check
    }
  else if ((d+f) > str_size)
    {
      unsigned fp, dp;
      fp = str_size/3;
      if (fp > f)
	fp = f;
      dp = str_size-fp-6;
      if (dp > d)
	dp = d;
      fp = str_size-dp-6;
      if (fp > f)
	fp = f;
      dp = str_size-fp-6;
      if (dp > d)
	dp = d;

      memcpy(str, dir, dp);
      str += dp;
      memcpy(str, " ... ", 5);
      str += 5;
      memcpy(str, fname + f - fp, fp+1);
      str += fp;
    }
  else
    {
      memcpy(str, dir, d);
      memcpy(str+d, fname, f);
      str += d+f;
    }

  sprintf(str, ":%d", line);
}

// register lines for a specific task (called by user mode application)
PUBLIC static
void
Jdb_lines::register_lines(unsigned addr, unsigned task)
{
  // sanity check
  if (task >= 2048)
    {
      printf("register_lines: task value %d out of range (0-2047)\n", task);
      return;
    }
  
  if (addr == 0)
    {
      // disable lines for specific task
      task_lines[task].str = 0;
      return;
    }
  
  task_lines[task].str = (line_t*)Kmem::phys_to_virt(addr);
  
  unsigned min_addr = 0xffffffff, max_addr = 0;
  line_t *lin;

  // search lines with lowest / highest address
  for (lin = task_lines[task].str; lin->addr || lin->line; lin++)
    {
      if (lin->line < 0xfffd)
	{
	  unsigned addr = lin->addr;
	  if (addr < min_addr)
	    min_addr = addr;
	  if (addr > max_addr)
	    max_addr = addr;
	}
    }
  
  task_lines[task].beg = min_addr;
  task_lines[task].end = max_addr;
}

