INTERFACE:

#include "l4_types.h"

class Jdb_lines
{
private:
  typedef struct
    {
      unsigned int   addr;
      unsigned short line;
    } __attribute__ ((packed)) Line;
  
  typedef struct
    {
      Line     *str;
      Address  beg;
      Address  end;
    } Task_lines;
  
  static Task_lines task_lines[L4_uid::MAX_TASKS];
};


IMPLEMENTATION:

#include <cstdio>
#include <cstring>
#include "kmem.h"

Jdb_lines::Task_lines Jdb_lines::task_lines[L4_uid::MAX_TASKS];

// search line name that matches for a specific address
PUBLIC static
bool
Jdb_lines::match_addr_to_line(Address addr, Task_num task,
			      char *line, unsigned line_size,
			      int show_header_files)
{
  if (task>=L4_uid::MAX_TASKS)
    return false;

  const char *str = (const char*)task_lines[task].str;

  if (   (str != 0)
      && (task_lines[task].beg <= addr)
      && (task_lines[task].end >= addr))
    {
      int show_file = 1;
      Line *lin;
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
	  else if ((lin->addr == addr) && show_file)
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
Jdb_lines::match_address_to_line_fuzzy(Address addr, Task_num task,
				       char *line, unsigned line_size,
				       int show_header_files)
{
  if (task>=L4_uid::MAX_TASKS)
    return false;

  const char *str = (const char*)task_lines[task].str;

  if (   (str != 0)
      && (task_lines[task].beg <= addr)
      && (task_lines[task].end >= addr))
    {
      int show_file = 1;
      Address best_addr = (Address)-1, best_line = 0;
      Line *lin;
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
	  else if ((lin->addr <= addr)
		   && (addr-lin->addr < best_addr)
		   && show_file)
	    {
	      best_addr = addr-lin->addr;
	      best_dir  = dir;
	      best_file = file;
	      best_line = lin->line;
	    }
	}
      if (best_addr < (Address)-1)
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
  while (d>1 && f>2 && fname[0]=='.' && fname[1]=='.' && fname[2]=='/')
    {
      // cut last directory
      while ((d>1) && (dir[d-2]!='/'))
	d--;
      d--;
      // cut first ../ directory of fname
      fname+=3;
      f-=3;
    }

  str_size -= 6;		// for line number
  
  if (str_size < 10)
    {
      *str = '\0';		// sanity check
    }
  else if ((d+f) > str_size)
    {
      // abbreviate line in a sane way
      memcpy(str, "... ", 4);
      str      += 4;
      str_size -= 4;

      if (str_size > f)
	{
	  unsigned dp = str_size-f;
	  memcpy(str, dir+d-dp, dp);
	  str      += dp;
	  str_size -= dp;
	}
      memcpy(str, fname+f-str_size, str_size);
      str += str_size;
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
Jdb_lines::register_lines(Address addr, Task_num task)
{
  // sanity check
  if (task >= L4_uid::MAX_TASKS)
    {
      printf("register_lines: task value %d out of range (0-%d)\n", 
	  task, L4_uid::MAX_TASKS-1);
      return;
    }
  
  if (addr == 0)
    {
      // disable lines for specific task
      task_lines[task].str = 0;
      return;
    }
  
  task_lines[task].str = (Line*)Kmem::phys_to_virt(addr);
  
  Address min_addr = 0xffffffff, max_addr = 0;
  Line *lin;

  // search lines with lowest / highest address
  for (lin = task_lines[task].str; lin->addr || lin->line; lin++)
    {
      if (lin->line < 0xfffd)
	{
	  Address addr = lin->addr;
	  if (addr < min_addr)
	    min_addr = addr;
	  if (addr > max_addr)
	    max_addr = addr;
	}
    }
  
  task_lines[task].beg = min_addr;
  task_lines[task].end = max_addr;
}

