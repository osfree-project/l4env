
%{

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <l4/sys/consts.h>

#include "cfg.h"

#undef DEBUG

#define yyparse cfg_parse
#define yylex_destroy cfg_destroy

unsigned int cfg_verbose;
unsigned int cfg_fiasco_symbols = 1;
unsigned int cfg_fiasco_lines = 1;
char cfg_binpath[L4ENV_MAXPATH];
char cfg_libpath[L4ENV_MAXPATH];
char cfg_modpath[L4ENV_MAXPATH];
int  cfg_binpath_set;
int  cfg_libpath_set;

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

%token <string>  TASK TEMPLATE MODULE BINPATH LIBPATH MODPATH
%token <string>  VERBOSE MEMDUMP SLEEP MEMORY IOPORT
%token <string>  IN IS AT MB KB MS S MIN H POO POOL NOSUPERPAGES
%token <string>  FIASCO_SYMBOLS FIASCO_LINES KERNEL_QUOTA USE_QUOTA
%token <string>  DIRECT_MAPPED CONTIGUOUS DMAABLE ALLOW_VGA ALLOW_BIOS ALLOW_KILL
%token <string>  PRIORITY MCP ALLOW_CLI FILE_PROVIDER DS_MANAGER CAP_HANDLER
%token <string>  NO_SIGMA0 SHOW_APP_AREAS ALL_SECTS_WRITABLE TASKNO
%token <string>  UNSIGNED STRING L4ENV_BINARY ALLOW_IPC DENY_IPC
%token <string>  INTEGRITY_PARENT_ID INTEGRITY_ID HASH_MODULES INTEGRITY_SERVICE

%type <number>   number memnumber memmodifier task_flag
%type <number>   memflagspec memflags memflag
%type <number>   time timemodifier
%type <string>   string
%type <interval> memrange
%type <interval> iorange

%start file

/* grammar rules follow */
%%

file 		: rules
		;

rules		: rule rules
		|
		;

rule		: task_spec
		| task_constraint
		| global_setting
		;

global_setting	: VERBOSE number
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
			  cfg_binpath_set = ($2 != NULL);
			  if (!cfg_libpath_set)
			    strcpy_check(cfg_libpath, $2, sizeof(cfg_libpath));
			  if (cfg_verbose>1)
			    printf("binary path <%s>\n", cfg_binpath);
			  free($2);
			}
		| LIBPATH string
			{
			  strcpy_check(cfg_libpath, $2, sizeof(cfg_libpath));
			  cfg_libpath_set = ($2 != NULL);
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
		| KERNEL_QUOTA string number
			{
			  if (add_kquota($2, $3))
			    {
			      yyerror("Cannot add quota.");
			      YYABORT;
			    }
			}
		;

task_spec	: TASK string string
			{
			  if (cfg_new_task($2, $3))
			    {
			      yyerror("Error creating cfg_task");
			      YYERROR;
			    }
			}
		| TASK string
			{
			  if (cfg_new_task($2, NULL))
			    {
			      yyerror("Error creating cfg_task");
			      YYERROR;
			    }
			}
		| TASK TEMPLATE
			{
			  cfg_new_task_template();
			}
		;

task_constraint	: task_modspec
		| MEMORY memconstraint
			{
			  // MEMORY is a string, therefore we need an empty
			  // rule to avoid return value clashes. bison assumes
			  // that if no return value is given the return values
			  // of one of the rule-elements should be used...
			}
		| IOPORT ioconstraint
			{
			}
                | TASKNO number
                        {
                          if (cfg_task_no($2))
                            {
                              yyerror("Error setting task numer");
                              YYABORT;
                            }
                        }
		| PRIORITY number
			{ 
			  if (cfg_task_prio($2))
			    {
			      yyerror("Error setting task priority");
			      YYABORT;
			    }
			}
		| MCP number
			{
			  if (cfg_task_mcp($2))
			    {
			      yyerror("Error setting task mcp");
			      YYABORT;
			    }
			}
		| FILE_PROVIDER string
			{
			  if (cfg_task_fprov($2))
			    {
			      yyerror("Error setting file provider");
			      YYABORT;
			    }
			}
		| DS_MANAGER string
			{
			  if (cfg_task_dsm($2))
			    {
			      yyerror("Error setting dataspace manager");
			      YYABORT;
			    }
			}
                | CAP_HANDLER string
                        {
                          if (cfg_task_caphandler($2))
                            {
                              yyerror("Error setting capability fault handler.");
                              YYABORT;
                            }
                        }
                | ALLOW_IPC string
                        {
                          if (cfg_task_ipc($2, CAP_TYPE_ALLOW))
                            {
                              yyerror("Error allowing IPC.");
                              YYABORT;
                            }
                        }
                | DENY_IPC string
                        {
                          if (cfg_task_ipc($2, CAP_TYPE_DENY))
                            {
                              yyerror("Error denying IPC.");
                              YYABORT;
                            }
                        }
		| INTEGRITY_ID string
			{
                          if (cfg_task_integrity_id($2, CFG_INTEGRITY_ID))
                            {
                              yyerror("Error setting integrity ID.");
                              YYABORT;
                            }
			}
		| INTEGRITY_PARENT_ID string
			{
                          if (cfg_task_integrity_id($2, CFG_INTEGRITY_PARENT_ID))
                            {
                              yyerror("Error setting integrity ID for parent.");
                              YYABORT;
                            }
			}
		| INTEGRITY_SERVICE string
			{
                          if (cfg_task_integrity_id($2, CFG_INTEGRITY_SERVICE_ID))
                            {
                              yyerror("Error setting integrity service.");
                              YYABORT;
                            }
			}
		| task_flag
			{
			  if (cfg_task_flag($1))
			    {
			      yyerror("Error setting task flag");
			      YYABORT;
			    }
			}
		| USE_QUOTA string
			{
			  if (cfg_task_kquota($2))
			    {
			      yyerror("Error setting kernel memory quota.");
			      YYABORT;
			    }
			}
		;

task_flag	: DIRECT_MAPPED
			{ $$ = CFG_F_DIRECT_MAPPED; }
		| NOSUPERPAGES
			{ $$ = CFG_F_NOSUPERPAGES; }
		| ALLOW_KILL
			{ $$ = CFG_F_ALLOW_KILL; }
		| ALLOW_VGA
			{ $$ = CFG_F_ALLOW_VGA; }
		| ALLOW_BIOS
			{ $$ = CFG_F_ALLOW_BIOS; }
		| NO_SIGMA0
			{ $$ = CFG_F_NO_SIGMA0; }
		| ALLOW_CLI
			{ $$ = CFG_F_ALLOW_CLI; }
		| SHOW_APP_AREAS
			{ $$ = CFG_F_SHOW_APP_AREAS; }
		| ALL_SECTS_WRITABLE
			{ $$ = CFG_F_ALL_WRITABLE; }
		| HASH_MODULES
			{ $$ = CFG_F_HASH_BINARY | CFG_F_HASH_MODULES; }
		| L4ENV_BINARY
			{ $$ = CFG_F_L4ENV_BINARY; }
		;

task_modspec	: MODULE string string memrange
			{ 
			  if (cfg_new_module($2, $3, $4.low, $4.high))
			    {
			      yyerror("Error adding module");
			      YYABORT;
			    }
			}
		| MODULE string memrange
			{ 
			  if (cfg_new_module($2, NULL, $3.low, $3.high))
			    {
			      yyerror("Error adding module");
			      YYABORT;
			    }
			}
		;

memconstraint	: memnumber memrange memflagspec
			{
			  if (cfg_new_mem($1, $2.low, $2.high, $3))
			    {
			      yyerror("Error adding memory region");
			      YYABORT;
			    }
			}
		;

memnumber	: number memmodifier
			{ $$ = $1*$2; }
		;

memmodifier	: MB	{ $$ = 1024*1024; }
		| KB	{ $$ = 1024; }
		|	{ $$ = 1; }
		;

memrange	: IN '[' memnumber ',' memnumber ']'
			{ $$.low = $3; $$.high = $5; }
		| AT memnumber
			{ $$.low = $2; $$.high = 0; }
		|
			{ $$.low = 0; $$.high = L4_MAX_ADDRESS; }
		;

memflagspec	: IS '[' memflags ']'
			{ $$ = $3; }
		|
			{ $$ = 0; }
		;

memflags	: memflags memflag
			{ $$ = $1 | $2; }
		| memflags POOL number
			{ $$ = ($1 & 0xffff) | ($3 << 16); }
		| memflag
			{ $$ = $1; }
		;

memflag		: DMAABLE
			{ $$ = CFG_M_DMA_ABLE; }
		| CONTIGUOUS
			{ $$ = CFG_M_CONTIGUOUS; }
		| DIRECT_MAPPED
			{ $$ = CFG_M_DIRECT_MAPPED; }
		| NOSUPERPAGES
			{ $$ = CFG_M_NOSUPERPAGES; }
		;

ioconstraint	: iorange
			{
			  if (cfg_new_ioport($1.low, $1.high))
			    {
			      yyerror("Error adding ioport region");
			      YYABORT;
			    }
			}
		;

iorange		: '[' number ',' number ']'
			{ $$.low = $2; $$.high = $4; }
		| number
			{ $$.low = $$.high = $1; }
		;

time		: number timemodifier
      			{ $$ = $1*$2; }
		;

timemodifier	: S	{ $$ = 1000; }
		| MS	{ $$ = 1; }
		| MIN	{ $$ = 60000; }
		| H	{ $$ = 3600000; }
		|	{ $$ = 1000; }
		;

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

