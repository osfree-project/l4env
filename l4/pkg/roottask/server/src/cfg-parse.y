/* -*- indented-text -*- */

%{
#include <unistd.h>

#include <l4/rmgr/proto.h>
#include <l4/sys/kdebug.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "bootquota.h"
#include "quota.h"
#include "init.h"
#include "rmgr.h"
#include "cfg.h"
#include "types.h"

#define yyparse __cfg_parse

static void yyerror(const char *s);

int  __cfg_task               = -1; /* the task to configure */
int  __cfg_mod                = -1; /* the module to configure */
char __cfg_name[MOD_NAME_MAX] = ""; /* the name of the task we configue */

quota_t     q; /* quota buffer for configured task */
bootquota_t b; /* bootquota buffer for configured task */

static unsigned c_max = -1, c_low = 0, c_high = -1, c_mask = 0xffffffff;

#ifndef min
#define min(x,y) ((x)<(y)?(x):(y))
#endif
#ifndef max
#define max(x,y) ((x)>(y)?(x):(y))
#endif

#define minset(x, y) ((x) = min((x),(y)))
#define maxset(x, y) ((x) = max((x),(y)))

#define SET_DEBUG_OPT(proto, action) 					\
	do {								\
		debug_log_mask |= (1L << proto);			\
		if (action)						\
			debug_log_types = (2L | (action << 16) | 	\
					   (debug_log_types & 7));	\
	} while(0);
%}

%union {
  char *string;
  unsigned number;
  struct {
  unsigned low, high;
  } interval;
}

%token TASK MODNAME CHILD IRQ MAX IN MASK MEMORY HIMEM MEM_OFFSET ALLOW_CLI
%token LOGMCP BOOTMCP BOOTPRIO BOOTWAIT RMGR SIGMA0 DEBUGFLAG VERBOSE LOG
%token SMALLSIZE SMALL BOOTSMALL MODULE MODULES ATTACHED
%token TASK_PROTO TASK_ALLOC TASK_GET TASK_FREE
%token TASK_CREATE TASK_DELETE TASK_SMALL
%token TASK_GET_ID TASK_CREATE_WITH_PRIO
%token MEM_PROTO MEM_FREE MEM_FREE_FP
%token IRQ_PROTO IRQ_GET IRQ_FREE
%token RMGR_PROTO RMGR_PING
%token LOG_IT KDEBUG
%token <string> UNSIGNED STRING
%type <number> number
%type <interval> setspec
%type <string> string

/* grammer rules follow */
%%

file	: globalconstraint rules
	;

rules	: rule rules
	|
	;

rule	: taskspec constraints modules
		  {
		    printf("  configured task 0x%02x (%s):\n",
		      __cfg_task, __cfg_name);
		    if (!quota_is_default_mem(&q))
		      printf("    memory:    [%lx,%lx,%lx]\n",
			q.mem.low, q.mem.high, q.mem.max);
		    if (!quota_is_default_himem(&q))
		      printf("    high mem:  [%lx,%lx,%lx]\n",
			q.himem.low, q.himem.high, q.himem.max);
		    if (!quota_is_default_task(&q))
		      printf("    task:      [%x,%x,%x]\n",
			q.task.low, q.task.high, q.task.max);
		    if (!quota_is_default_small(&q))
		      printf("    small:     [%x,%x,%x]\n",
			q.small.low, q.small.high, q.small.max);
		    if (!quota_is_default_misc(&q) ||
		        !bootquota_is_default(&b))
		      printf("    vm_offs:%x irq:%04x lmcp:%04x allow_cli:%x"
				" mcp:%x prio:%x small:%x mods:%d\n",
			q.offset, q.irq, q.log_mcp, q.allow_cli,
			b.mcp, b.prio, b.small_space, b.mods);
		    if (__cfg_task)
		      {
		        bootquota_set(__cfg_task, &b);
			quota_set(__cfg_task, &q);
		      }
		    else
		      {
		        const char *n;
		        n = cfg_quota_set(__cfg_name, &q);
		        cfg_bootquota_set(n, &b);
		      }
		    __cfg_mod = -1;
		    strcpy(__cfg_name, "");
		    quota_set_default(&q);
		    bootquota_set_default(&b);
		  }

	| smallsizerule
	| flag
	;

smallsizerule : SMALLSIZE number{ small_space_size = $2; };

numerical_options: number number{ debug_log_mask = $1;
				  debug_log_types = $2; };

verbose_option: TASK_PROTO	{ SET_DEBUG_OPT(RMGR_TASK, 0); }
	     | TASK_ALLOC	{ SET_DEBUG_OPT(RMGR_TASK, RMGR_TASK_ALLOC); }
	     | TASK_GET		{ SET_DEBUG_OPT(RMGR_TASK, RMGR_TASK_GET); }
	     | TASK_FREE	{ SET_DEBUG_OPT(RMGR_TASK, RMGR_TASK_FREE); }
	     | TASK_CREATE	{ SET_DEBUG_OPT(RMGR_TASK, RMGR_TASK_CREATE); }
	     | TASK_DELETE	{ SET_DEBUG_OPT(RMGR_TASK, RMGR_TASK_DELETE); }
	     | TASK_SMALL	{ SET_DEBUG_OPT(RMGR_TASK, RMGR_TASK_SET_SMALL); }
	     | TASK_GET_ID	{ SET_DEBUG_OPT(RMGR_TASK, RMGR_TASK_GET_ID); }
	     | TASK_CREATE_WITH_PRIO	
	     			{ SET_DEBUG_OPT(RMGR_TASK, RMGR_TASK_CREATE_WITH_PRIO); }
	     | MEM_PROTO	{ SET_DEBUG_OPT(RMGR_MEM, 0); }
	     | MEM_FREE		{ SET_DEBUG_OPT(RMGR_MEM, RMGR_MEM_FREE); }
	     | MEM_FREE_FP	{ SET_DEBUG_OPT(RMGR_MEM, RMGR_MEM_FREE_FP); }
	     | IRQ_PROTO	{ SET_DEBUG_OPT(RMGR_IRQ, 0); }
	     | IRQ_GET		{ SET_DEBUG_OPT(RMGR_IRQ, RMGR_IRQ_GET); }
	     | IRQ_FREE		{ SET_DEBUG_OPT(RMGR_IRQ, RMGR_IRQ_FREE); }
	     | RMGR_PROTO	{ SET_DEBUG_OPT(RMGR_IRQ, 0); }
	     | RMGR_PING	{ SET_DEBUG_OPT(RMGR_IRQ, RMGR_RMGR_PING); }
	     | LOG_IT		{ debug_log_types |= 1; }
	     | KDEBUG		{ debug_log_types |= 4; }
	     ;

verbose_options: verbose_option verbose_options
	     | verbose_option
	     ;

log_options:   numerical_options
	     | verbose_options
	     ;

flag	: BOOTWAIT		{ }
	| DEBUGFLAG		{ }
	| LOG log_options 
	;

globalconstraint :
	| LOGMCP number		{ quota_init_log_mcp($2); }
	| BOOTMCP number	{ quota_init_mcp($2); }
	| BOOTPRIO number	{ quota_init_prio($2); }
	;

modules	: module modules
	| ATTACHED number MODULES { __cfg_mod += $2;
				    b.mods += $2; }
	|
	;

module	: nextmod asserts	{ b.mods++; }
	;

nextmod	: MODULE		{ __cfg_mod++; }
	;

taskspec : TASK			{ __cfg_task++; }
	| TASK taskname	
	| TASK RMGR		{ __cfg_task = TASKNO_ROOT; }
	| TASK SIGMA0		{ __cfg_task = TASKNO_SIGMA0; }
	| TASK number		{ __cfg_task = $2; }
	;

taskname: MODNAME string
		{
		  __cfg_task = 0;
		  snprintf(__cfg_name, sizeof(__cfg_name), "%s", $2);
		  find_module(&__cfg_mod, __cfg_name);
                  free($2);
		}

asserts : assert asserts
	|
	;

assert	: MODNAME string	
		{
		  check_module(__cfg_mod, $2);
		  free($2);
		}
	;

constraints : constraint constraints
	| 
	;

constraint : CHILD childconstraints	{ 
					  maxset(q.task.low, c_low);
					  minset(q.task.high, c_high);
					  minset(q.task.max, c_max);
                                          c_low = 0; 
					  c_high = -1; 
					  c_max = -1; 
					}
	| MEMORY memoryconstraints	{ 
					  maxset(q.mem.low, c_low);
					  minset(q.mem.high, c_high);
					  minset(q.mem.max, c_max);
                                          c_low = 0; 
					  c_high = -1; 
					  c_max = -1; 
					}
	| HIMEM memoryconstraints	{ 
					  maxset(q.himem.low, c_low);
					  minset(q.himem.high, c_high);
					  minset(q.himem.max, c_max);
                                          c_low = 0; 
					  c_high = -1; 
					  c_max = -1; 
					}
	| MEM_OFFSET number		{
					  q.offset = $2;
					}
	| IRQ irqconstraints		{ 
					  unsigned mask = 0, i;
					  for (i  = max( 0, c_low);
					       i <= min(15, c_max); i++)
					    {
                                              mask |= 1L << i;
					    }
					  q.irq &= c_mask & mask;
                                          c_low = 0; 
					  c_high = -1; 
					  c_max = -1;
					  c_mask = 0xffffffff; 
					}
	| SMALL smallconstraints	{ 
					  maxset(q.small.low, c_low);
					  minset(q.small.high, c_high);
					  minset(q.small.max, c_max);
                                          c_low = 0; 
					  c_high = -1; 
					  c_max = -1; 
					}
	| LOGMCP number			{ q.log_mcp = $2; }
	| BOOTMCP number		{ b.mcp = $2; }
	| BOOTPRIO number		{ b.prio = $2; }
	| BOOTSMALL number		{ b.small_space = $2; }
	| ALLOW_CLI			{ q.allow_cli = 1; }
	;

childconstraints : numconstraints
	;

smallconstraints : numconstraints
	;

memoryconstraints : numconstraints
	;

irqconstraints : numconstraints
	| maskconstraints
	;

numconstraints : numconstraint numconstraints
	| numconstraint
	;

numconstraint : MAX number		{ c_max = min(c_max, $2); }
	| IN setspec			{ c_low = max(c_low, $2.low); 
					  c_high = min(c_high, $2.high); }
	;

setspec : '[' number ',' number ']'	{ $$.low = $2; $$.high = $4; }
	;

maskconstraints : maskconstraint
	;

maskconstraint : MASK number		{ c_mask &= $2; }
	;

number	: UNSIGNED			{ $$ = strtoul($1, 0, 0); }
	;

string	: STRING			{ $$ = strdup($1 + 1);
					  $$[strlen($$) - 1] = 0; };

%%
/* end of grammer -- misc C source code follows */

#include "cfg-scan.c"

static void yyerror(const char *s)
{
  printf("ERROR: while parsing config file: %s\n"
	 "       at line %d, col %d\n", s, line, col);
}


#ifdef TEST

char cfg[] = "task modname 'foo'\n"
             "#a comment\n"
             "child in [10,30] max 100\n";


int main(void)
{
  cfg_setup_input(cfg, cfg + sizeof(cfg));
  return cfg_parse();
}

#endif /* TEST */
