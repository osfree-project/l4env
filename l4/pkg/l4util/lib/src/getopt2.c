#include <l4/util/string.h>
#include <l4/util/getopt.h>

/* extracted from OSKIT */
#define MULTIBOOT_CMDLINE       (1L<<2)
struct multiboot_info
{
  unsigned	flags;
  unsigned	dummy[3];
  char*		cmdline;
};
/* end of OSKIT stuff */


#define MAXARGC 20
#define MAXENVC 30
static char argbuf[1024];
char *_argv[MAXARGC];
int  _argc = 0;

#define isspace(c) ((c)==' '||(c)=='\t'||(c)=='\r'||(c)=='\n')

static void
parse_args(char *argbuf)
{
  char *cp;

  /* make _argc, _argv */
  _argc = 0;
  cp = argbuf;
  while (*cp && _argc < MAXARGC-1)
    {
      while (*cp && isspace(*cp))
	cp++;

      if (*cp)
	{
	  _argv[_argc++] = cp;
	  while (*cp && !isspace(*cp))
	    cp++;

	  if (*cp)
	    *cp++ = '\0';
	}
    }
  _argv[_argc] = (void*) 0;
};

void
arg_init(char* cmdline)
{
  if (cmdline)
    {
      strncpy(argbuf, cmdline,
	      sizeof(argbuf) < strlen(cmdline) ?
	      sizeof(argbuf) : 1+strlen(cmdline));
      parse_args(cmdline);
    };
};

void _main(struct multiboot_info *mbi, unsigned int flag);

void 
_main(struct multiboot_info *mbi, unsigned int flag)
{
  if (flag && mbi && (mbi->flags & MULTIBOOT_CMDLINE))
    arg_init((char*) mbi->cmdline);
};
