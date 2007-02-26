/*
  deptrack.c -- high-level routines for gendepend.

  (c) Han-Wen Nienhuys <hanwen@cs.uu.nl> 1998
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <regex.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

#include "gendep.h"

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

extern int errno;

static FILE * output;
static struct
{
  regex_t *re_array;
  char *invert_array;
  int max, no;
} regexps;

static char * executable_name;
static char * wanted_executable_name;
static char * target;
static char * depfile_name;

/* List of dependencies. We first collect all, then write them out. */
static struct strlist{
	const char*name;
	struct strlist *next;
} *dependencies; 

#define STRLEN 1024

/*
  simplistic wordwrap
 */
static void
write_word (char const * word)
{
  static int column;
  int l = strlen (word);
  if (l + column > 75 && column>1)
    {
      fputs ("\\\n\t",output);
      column = 8;
    }

  column += l;
  fputs (word, output);
  fputs (" ", output);
}

static void
xnomem(void *p)
{
  if (!p)
    {
      errno = ENOMEM;
      perror ("libgendep.so");
    }
}

static char *
xstrdup (char *s)
{
  char *c = strdup (s);
  xnomem (c);
  return c;
}

static void *
xrealloc (void *s, int sz)
{
  void *c = realloc (s, sz);
  xnomem (c);
  return c;
}

static void *
xmalloc (int sz)
{
  void *c;
  c = malloc (sz);
  xnomem (c);
  return c;
}

/*!\brief Trim the filename
 *
 * Remove redundant "./" at the beginning of a filename.
 *
 * We are not very minimal about memory usage, because this would require
 * scanning the filename twice, instead of once.
 */
static char* trim (const char*fn){
  char *name;

  /* remove trailing "./" */
  while(fn[0]=='.' && fn[1]=='/') fn+=2;
  name = xmalloc(strlen(fn)+1);
  if(name==0) return 0;

  strcpy(name, fn);
  return name;
}

/*!\brief add a dependency
 *
 * Add a dependency to the list of dependencies for the current target.
 * If the name is already registered, it wount be added.
 */
static void gendep__adddep(const char*name){
  struct strlist *dep;
  char *namecopy;

  if((namecopy = trim(name))==0) return;
  if(!strcmp(target, name)) return;
  for(dep=dependencies; dep; dep=dep->next){
  	if(!strcmp(dep->name, namecopy)){
	    free(namecopy);
	    return;
	}
  }
  dep = xmalloc(sizeof(struct strlist));
  dep->name = namecopy;
  dep->next = dependencies;
  dependencies = dep;
}

/*!\brief Delete a dependency
 *
 * Delete a dependency from the list of dependencies for the current target.
 * This is used if an already registered file is removed later.
 */
static void gendep__deldep(const char*name){
  struct strlist *dep, *old;
  char *namecopy;
  
  old = 0;
  if((namecopy = trim(name))==0) return;
  for(dep=dependencies; dep; dep=dep->next){
  	if(!strcmp(dep->name, namecopy)){
	    if(old) old->next = dep->next;
	    else dependencies=dep->next;
	    free(namecopy);
	    free((char*)dep->name);
	    free(dep);
	    return;
	}
	old = dep;
  }
  free(namecopy);
}

static int
gendep_getenv (char **dest, char *what)
{
  char var[STRLEN]="GENDEP_";
  strcat (var, what);
  *dest = getenv (var);

  return !! (*dest);
}

/*
  Do hairy stuff with regexps and environment variables.
 */
static void
setup_regexps (void)
{
  char *regexp_val =0;
  char *end, *start;

  if (!gendep_getenv (&wanted_executable_name, "BINARY"))
    return;

  if (strcmp (wanted_executable_name, executable_name))
    {
      return;
    }

  gendep_getenv(&depfile_name, "DEPFILE");

  if (!gendep_getenv (&target, "TARGET"))
    return;

  if (!gendep_getenv (&regexp_val, executable_name))
    return;

  /*
    let's hope malloc (0) does the sensible thing (not return NULL, that is)
  */
  regexps.re_array = xmalloc (0);
  regexps.invert_array = xmalloc (0);
  
  regexp_val = xstrdup (regexp_val);
  start = regexp_val;
  end = regexp_val + strlen (regexp_val);

  /*
    Duh.  The strength of C : string handling. (not).   Pull apart the whitespace delimited
    list of regexps, and try to compile them.
    Be really gnuish with dynamic allocation of arrays. 
   */
  do {
    char * end_re = 0;
    int in_out;
    while (*start && *start != '+' && *start != '-')
      start ++;

    in_out= (*start == '+') ;
    start ++;
    end_re = strchr (start, ' ');
    if (end_re)
      *end_re = 0;

    if (*start)
      {
	regex_t regex;
	int result= regcomp (&regex, start, REG_NOSUB);

	if (result)
	  {
	    fprintf (stderr, "libgendep.so: Bad regular expression `%s\'\n", start);
	    goto duh;		/* ugh */
	  }

	if (regexps.no >= regexps.max)
	  {
	    /* max_regexps = max_regexps * 2 + 1; */
	    regexps.max ++;
	    regexps.re_array = xrealloc (regexps.re_array, regexps.max * sizeof (regex_t));
	    regexps.invert_array = xrealloc (regexps.invert_array, regexps.max * sizeof (char));	
	  }

	regexps.re_array[regexps.no] = regex;
	regexps.invert_array[regexps.no++] = in_out;
      }

  duh:
    start = end_re ? end_re + 1 :  end;
  } while (start < end);
}

/*
  Try to get the name of the binary.  Is there a portable way to do this?
 */
static void get_executable_name (void)
{
  FILE *cmdline = fopen ("/proc/self/cmdline", "r");
  char cmd[STRLEN];
  int i=0;
  char *basename_p;
  int c;
  cmd[STRLEN-1] = 0;
  
  while ((c = fgetc(cmdline))!=EOF && c && i < STRLEN-1)
    cmd[i++] = c;

  cmd[i++] = 0;

  /* ugh.  man 3 basename -> ?  */
  basename_p =  strrchr (cmd, '/');
  if (basename_p)
    basename_p++;
  else
    basename_p = cmd;

  executable_name = xstrdup (basename_p);
}

static void initialize (void) __attribute__ ((constructor));

static void initialize (void)
{
  get_executable_name ();
  setup_regexps ();

  if (target)
    {
      char fn[STRLEN];
      char *slash;

      fn[0] = '\0';
      if(depfile_name){
          strncpy(fn, depfile_name, STRLEN);
      } else {
          slash = strrchr(target, '/');
          // copy the path
          strncat (fn, target, min(STRLEN, slash?slash-target+1:0));
          strncat (fn, ".", STRLEN);
          // copy the name
          strncat(fn, slash?slash+1:target, STRLEN);
          strncat (fn, ".d", STRLEN);
      }
      fn[STRLEN-1]=0;
      
      if((output = fopen (fn, "w"))==0){
        fprintf(stderr, "libgendep.so: cannot open %s\n", fn);
        return;
      }
      write_word (target);
      write_word (":");
    }
}

/* Someone is opening a file.  If it is opened for reading, and
  matches the regular expressions, write it to the dep-file. 
  
  Note that we explicitly ignore accesses to /etc/mtab and /proc/...
  as these files are inspected by libc for Linux and tend to change.
 */
void
gendep__register_open (char const *fn, int flags)
{
  if (output && !(flags & (O_WRONLY | O_RDWR)))
    {
      int i;

      if (fn == strstr(fn, "/etc/mtab") ||
	  fn == strstr(fn, "/proc/"))
	return;

      for (i =0; i<  regexps.no; i++)
	{
	  int not_matched = regexec (regexps.re_array +i, fn, 0, NULL, 0);

	  if (!(not_matched ^ regexps.invert_array[i]))
	    return;
	}

      gendep__adddep(fn);
    }
}

void
gendep__register_unlink (char const *fn)
{
  gendep__deldep(fn);
}


static void finish (void) __attribute__ ((destructor));

static void finish (void)
{
  if (output)
    {
      struct strlist *dep;

      for(dep=dependencies; dep; dep=dep->next){
        write_word(dep->name);
      }
      fprintf (output, "\n");
      for(dep=dependencies; dep; dep=dep->next){
	fputs(dep->name, output);
        fputs(":\n", output);
      }
      fclose (output);
    }
}

