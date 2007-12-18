
%option never-interactive
%option noyywrap

%{

int yylex(void);
static void count(void);

/* we use a memory-based input buffer */
static int my_yyinput(char *buf, int max_size);
#undef YY_INPUT
#define YY_INPUT(b, r, ms) (r = my_yyinput(b, ms))
#define YY_NEVER_INTERACTIVE 1

/* make sure we don't use file I/O */
#undef ECHO
#define ECHO

#undef fprintf
#define fprintf(stream, args...) printf(args)

static unsigned line = 1, col = 1;
static char error_line_buffer[256];

%}

%start COMMENT

DIGIT		[0-9]
HEXDIGIT	[a-fA-F0-9]
SPACE		[\f\r\t\032 ]
NEWLINE		\n

/* lexing rules follow */

%%

#			BEGIN COMMENT;
<COMMENT>.*$		BEGIN 0;
task			{ count(); return TASK; }
template		{ count(); return TEMPLATE; }
module			{ count(); return MODULE; }
binpath			{ count(); return BINPATH; }
libpath			{ count(); return LIBPATH; }
modpath			{ count(); return MODPATH; }
verbose			{ count(); return VERBOSE; }
fiasco_symbols		{ count(); return FIASCO_SYMBOLS; }
fiasco_lines		{ count(); return FIASCO_LINES; }
memdump			{ count(); return MEMDUMP; }
sleep			{ count(); return SLEEP; }
taskno                  { count(); return TASKNO; }
memory			{ count(); return MEMORY; }
ioport			{ count(); return IOPORT; }
kb|Kb|kB|KB		{ count(); return KB; }
mb|Mb|mB|MB		{ count(); return MB; }
s			{ count(); return S; }
min			{ count(); return MIN; }
h			{ count(); return H; }
ms			{ count(); return MS; }
in			{ count(); return IN; }
is			{ count(); return IS; }
at			{ count(); return AT; }
direct_mapped		{ count(); return DIRECT_MAPPED; }
l4env			{ count(); return L4ENV_BINARY; }
contiguous		{ count(); return CONTIGUOUS; }
no_sigma0		{ count(); return NO_SIGMA0; }
allow_vga		{ count(); return ALLOW_VGA; }
allow_bios		{ count(); return ALLOW_BIOS; }
dma_able		{ count(); return DMAABLE; }
dmaable			{ count(); return DMAABLE; }
pool			{ count(); return POOL; }
nosuperpages		{ count(); return NOSUPERPAGES; }
priority		{ count(); return PRIORITY; }
file_provider		{ count(); return FILE_PROVIDER; }
ds_manager		{ count(); return DS_MANAGER; }
cap_handler             { count(); return CAP_HANDLER; }
allow_ipc               { count(); return ALLOW_IPC; }
deny_ipc                { count(); return DENY_IPC; }
kernel_quota            { count(); return KERNEL_QUOTA; }
use_quota               { count(); return USE_QUOTA; }
mcp			{ count(); return MCP; }
allow_cli		{ count(); return ALLOW_CLI; }
show_app_areas		{ count(); return SHOW_APP_AREAS; }
all_sects_writable	{ count(); return ALL_SECTS_WRITABLE; }
integrity_id		{ count(); return INTEGRITY_ID; }
integrity_parent_id	{ count(); return INTEGRITY_PARENT_ID; }
hash_modules    	{ count(); return HASH_MODULES; }
('[^']*'|\"[^\"]*\") 	{ count(); yylval.string = yytext; return STRING; }
(0[xX]{HEXDIGIT}+|{DIGIT}+) { count();
				   yylval.string = yytext; return UNSIGNED; }
{SPACE}			{ count(); }
{NEWLINE}		{ count(); }
.			{ count();
			  return yytext[0];
			}

%%

/* end of lexing rules */

static const char *myinput=0, *myinputptr=0, *myinputlim=0;

#ifndef min
#define min(x,y) ((x)<(y)?(x):(y))
#endif

static int
my_yyinput(char *buf, int max_size)
{
  int have_size = myinputlim-myinputptr;
  int n = min(max_size, have_size+1);
  
  if (n > 0)
    {
      if (n > have_size)
        {
	  /* make sure that the script is terminated */
          memcpy(buf, myinputptr, n-1);
	  buf[n-1] = '\n';
	}
      else
        memcpy(buf, myinputptr, n);
      myinputptr += n;
    }
  return n;
}

static void
count(void)
{
  int i;
  static char *elb = error_line_buffer;
  for (i=0; yytext[i] != '\0'; i++)
    {
      if (yytext[i] == '\n')
        {
          col = 1;
	  line++;
	  elb = error_line_buffer;
        }
      else
        {
	  if (elb - error_line_buffer < sizeof(error_line_buffer)-1)
	    *elb++ = yytext[i];
	  if (yytext[i] == '\t')
	    col += 8 - (col & (8-1));
	  else
	    col++;
	}
      *elb = 0;
    }
}

void
cfg_setup_input(const char *cfg_buffer, int size)
{
  myinput = myinputptr = cfg_buffer;
  myinputlim = cfg_buffer + size;
  line = 1;
  col = 1;
  yy_start = 0;
  /* Starting with flex 2.5.33, the meaning of yy_init was reversed. To
   * perform the cleanup we could call yylex_destroy but unfortunately
   * this function is not backward-compatible. */
#if YY_FLEX_MAJOR_VERSION*10000 + YY_FLEX_MINOR_VERSION*100 + YY_FLEX_SUBMINOR_VERSION < 20533
  yy_init = 1;
#else
  yy_init = 0;
#endif
}

void
cfg_done(void)
{
  yy_delete_buffer(YY_CURRENT_BUFFER);
}

static void
yyerror(const char *s)
{
  printf("line=%d col=%d: %s\n", line, col-1, s);
  printf("%s\n", error_line_buffer);
  printf("%*s", col, "^\n");
}

