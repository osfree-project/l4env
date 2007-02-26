
%{

#include <stdlib.h>
#include <string.h>

#include <l4/sys/consts.h>
#include <l4/env/env.h>

#include "cfg.h"

#undef DEBUG

#define yyparse cfg_parse

unsigned int cfg_verbose = 0;
unsigned int cfg_fiasco_symbols = 0;
unsigned int cfg_fiasco_lines = 0;
char cfg_binpath[L4ENV_MAXPATH] = { 0 };
char cfg_libpath[L4ENV_MAXPATH] = { 0 };
char cfg_modpath[L4ENV_MAXPATH] = { 0 };
int  cfg_binpath_set = 0;
int  cfg_libpath_set = 0;

static void
strcpy_check(char *dest, const char *src, unsigned int size)
{
  strncpy(dest, src, size-1);
  dest[size-1]='\0';
}

#ifdef DEBUG
#define YYDEBUG 1
char
*getenv(const char *name)
{
  return "9";
}
#endif

static void yyerror(const char *s);
int yyparse(void);

%}

%union {
  char *string;
  unsigned int number;
  struct {
    unsigned low, high;
  } interval;
}

%token <string> TASK MODULE BINPATH LIBPATH MODPATH
%token <string> VERBOSE MEMDUMP SLEEP MEMORY IN AT MB KB MS S MIN H
%token <string> FIASCO_SYMBOLS FIASCO_LINES
%token <string> DIRECT_MAPPED CONTIGUOUS DMA_ABLE REBOOT_ABLE NO_VGA PRIORITY
%token <string> NO_SIGMA_NULL
%token <string> UNSIGNED STRING

%type <number>	 number memnumber memmodifier taskflag taskflags
%type <number>	 memflag memflags time timemodifier
%type <string>	 string
%type <interval> memrange

%start file

/* grammar rules follow */
%%

file 		: rules
		;

rules		: rule rules
		|
		;

rule		: taskspec constraints modules
		| setting
		;

setting		: VERBOSE number
			{ cfg_verbose = $2; }
		| FIASCO_SYMBOLS number
			{ cfg_fiasco_symbols = $2; }
		| FIASCO_LINES number
			{ cfg_fiasco_lines = $2; }
		| MEMDUMP
			{
			  if (cfg_job(CFG_F_MEMDUMP, 0) < 0)
			    {
			      yyerror("Error creating cfg_job");
			      YYERROR;
			    }
			}
		| SLEEP time
			{
			  if (cfg_job(CFG_F_SLEEP, $2) < 0)
			    {
			      yyerror("Error creating cfg_job");
			      YYERROR;
			    }
			}
		| BINPATH string
			{ 
			  strcpy_check(cfg_binpath, $2, sizeof(cfg_binpath));
			  cfg_binpath_set = ($2 && *$2);
			  if (!cfg_libpath_set)
			    strcpy_check(cfg_libpath, $2, sizeof(cfg_libpath));
			  if (cfg_verbose>1)
			    printf("binary path <%s>\n", cfg_binpath);
			  free($2);
			}
		| LIBPATH string
			{
			  strcpy_check(cfg_libpath, $2, sizeof(cfg_libpath));
			  cfg_libpath_set = ($2 && *$2);
			  if (cfg_verbose>1)
			    printf("library path <%s>\n", cfg_libpath);
			  free($2);
			}
		| MODPATH string
			{
			  strcpy_check(cfg_modpath, $2, sizeof(cfg_modpath));
			  if (!cfg_libpath_set)
			    strcpy_check(cfg_libpath, $2, sizeof(cfg_libpath));
			  if (!cfg_binpath_set)
			    strcpy_check(cfg_binpath, $2, sizeof(cfg_binpath));
			  if (cfg_verbose>1)
			    printf("module path <%s>\n", cfg_modpath);
			  free($2);
			}
		;

taskspec	: TASK string string taskflags
			{
			  if (cfg_new_task($2, $3, $4))
			    {
			      yyerror("Error creating cfg_task");
			      YYERROR;
			    }
			}
		| TASK string taskflags
			{
			  if (cfg_new_task($2, NULL, $3))
			    {
			      yyerror("Error creating cfg_task");
			      YYERROR;
			    }
			}
		;

taskflags	: taskflag taskflags
			{ $$ = $1 | $2; }
		|
			{ $$ = 0; }
		;

taskflag	: DIRECT_MAPPED
			{ $$ = CFG_F_DIRECT_MAPPED; }
		| REBOOT_ABLE
			{ $$ = CFG_F_REBOOT_ABLE; }
		| NO_VGA
			{ $$ = CFG_F_NO_VGA; }
		| NO_SIGMA_NULL
			{ $$ = CFG_F_NO_SIGMA_NULL; }
		;

modules		: modspec modules
		|
		;

modspec		: MODULE string string
			{ 
			  if (cfg_new_module($2, $3))
			    {
			      yyerror("Error creating cfg_module");
			      YYABORT;
			    }
			}
		| MODULE string
			{ 
			  if (cfg_new_module($2, NULL))
			    {
			      yyerror("Error creating cfg_module");
			      YYABORT;
			    }
			}
		;

constraints	: constraint constraints
		|
		;

constraint	: MEMORY memconstraint
		| PRIORITY number
			{ cfg_set_prio($2); }
		;

memconstraint	: memnumber memrange memflags
			{ cfg_new_mem($1, $2.low, $2.high, $3); }
		;

memflags	: memflag memflags
			{ $$ = $1 | $2; }
		|
			{ $$ = 0; }
		;

memflag		: DMA_ABLE
			{ $$ = CFG_M_DMA_ABLE; }
		| CONTIGUOUS
			{ $$ = CFG_M_CONTIGUOUS; }
		| DIRECT_MAPPED
			{ $$ = CFG_M_DIRECT_MAPPED; }
		;


memrange	: IN '[' memnumber ',' memnumber ']'
			{ $$.low = $3; $$.high = $5; }
		| AT memnumber
			{ $$.low = $2; $$.high = 0; }
		|
			{ $$.low = 0; $$.high = L4_MAX_ADDRESS; }
		;

memnumber	: number memmodifier
			{ $$ = $1*$2; }
		;

memmodifier	: MB	{ $$ = 1024*1024; }
		| KB	{ $$ = 1024; }
		|	{ $$ = 1; }
		;

time		: number timemodifier
      			{ $$ = $1*$2; }

timemodifier	: S	{ $$ = 1000; }
		| MS	{ $$ = 1; }
		| MIN	{ $$ = 60000; }
		| H	{ $$ = 3600000; }
		|	{ $$ = 1000; }

number		: UNSIGNED
			{ $$ = strtoul($1, 0, 0); }
		;

string		: STRING
			{
			  if (!($$ = strdup($1 + 1)))
			    {
			      yyerror("Error allocating string");
			      YYABORT;
			    }
			  $$[strlen($$) - 1] = 0; 
			}
		;

%%
/* end of grammar */

#include "cfg-scan.c"

