
%option never-interactive

%{

int yylex(void);
static int yywrap(void);
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
module			{ count(); return MODULE; }
binpath			{ count(); return BINPATH; }
libpath			{ count(); return LIBPATH; }
modpath			{ count(); return MODPATH; }
verbose			{ count(); return VERBOSE; }
fiasco_symbols		{ count(); return FIASCO_SYMBOLS; }
fiasco_lines		{ count(); return FIASCO_LINES; }
memdump			{ count(); return MEMDUMP; }
sleep			{ count(); return SLEEP; }
memory			{ count(); return MEMORY; }
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
contiguous		{ count(); return CONTIGUOUS; }
reboot_able		{ count(); return REBOOTABLE; }
rebootable		{ count(); return REBOOTABLE; }
no_sigma0		{ count(); return NO_SIGMA_NULL; }
no_vga			{ count(); return NO_VGA; }
dma_able		{ count(); return DMAABLE; }
dmaable			{ count(); return DMAABLE; }
priority		{ count(); return PRIORITY; }
('[^']*'|\"[^\"]*\") 	{ count(); yylval.string = yytext; return STRING; }
(0[xX]{HEXDIGIT}+|{DIGIT}+) { count(); 
				   yylval.string = yytext; return UNSIGNED; }
{SPACE}			{ count(); }
{NEWLINE}		{ count(); }
.			{ count(); return yytext[0]; }

%%

/* end of lexing rules */

static const char *myinput=0, *myinputptr=0, *myinputlim=0;

#ifndef min
#define min(x,y) ((x)<(y)?(x):(y))
#endif

static int
my_yyinput(char *buf, int max_size)
{
  int n=min(max_size, myinputlim-myinputptr);
  
  if (n>0)
    {
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
  myinputlim = cfg_buffer + size - 1;
  
  line = 1; col = 1;

  yy_init = 1;
  yy_start = 0;
}

static int
yywrap(void)
{
  return 1;
}

static void
yyerror(const char *s)
{
  printf("line=%d col=%d: %s\n", line, col-1, s);
  printf("%s\n", error_line_buffer);
  printf("%*s", col, "^\n");
}

