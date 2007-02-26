/*
 * \brief   Flipsterm - an interactive FLIPS configuration tool
 * \date    2003-08-07
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <setjmp.h>
//#include <signal.h>

/*** L4 INCLUDES ***/
#include <l4/log/l4log.h>
#include <l4/dope/dopelib.h>
#include <l4/dope/term.h>
#include <l4/flips/libflips.h>

/*** LOCAL INCLUDES ***/
#include "cat.h"
#include "ifconfig.h"
#include "ip.h"
#include "httpstart.h"
#include "httpdump.h"

char LOG_tag[9] = "Flipsterm";
l4_ssize_t l4libc_heapsize = 1024*1024;

#define MAX_TOKENS       32
#define MAX_CMD_LEN     256
#define HISTORY_SIZE   1024

struct flips_cmd_struct {
	char *name;
	char *help;
	int  (*call) (int argc, char **argv);
};


void __libc_write(void);
void __libc_write(void) {
	printf("__libc_write called\n");
}

struct flips_cmd_struct commands[];

static int flipscmd_help(int argc, char **argv) {
	int i=0;
	term_printf("available commands:\n");
	while (commands[i].name != NULL) {
		term_printf("%-12s - %s\n",commands[i].name,commands[i].help);
		i++;
	}
	return 0;
}

static int flipscmd_quit(int argc, char **argv) {
	term_printf("have a nice day\n");
	return 0;
}

static int flipscmd_test(int argc, char **argv) {
	int i;
	term_printf("test command called - the arguments are:\n");
	for (i=0;i<argc;i++) {
		term_printf("arg%2d: %s\n",i,argv[i]);
	}
	return 0;
}

extern int flipscmd_ifconfig(int argc, char **argv);

struct flips_cmd_struct commands[] = {
	{"help",     "print information about the available commands",&flipscmd_help},
	{"quit",     "quit the Flips terminal",                &flipscmd_quit},
	{"test",     "test argument passing",                  &flipscmd_test},
	{"cat",      "display content of a file",              &flipscmd_cat},
	{"ifconfig", "configure a network interface",          &flipscmd_ifconfig},
	{"httpstart","start http server",                      &flipscmd_httpstart},
	{"httpdump", "dump http request",                      &flipscmd_httpdump},
	{NULL,NULL,NULL}
};



static int streq(char *s1,char *s2) {
	int i;
	if (!s1 || !s2) return 0;
	for (i=0;i<256;i++) {
		if (*(s1) != *(s2++)) return 0;
		if (*(s1++) == 0) return 1;
	}
	return 1;
}


/** UTILITY: FIND COMMAND THAT MATCHES TO THE GIVEN STRING
 * 
 * \return flips command structure or NULL if no matching command was found
 */
static struct flips_cmd_struct *find_cmd(char *cmdname) {
	int i=0;
	while (commands[i].name != NULL) {
		if (streq(commands[i].name, cmdname)) return &commands[i];
		i++;
	}
	return NULL;
}

/*****************/
/*** TOKENIZER ***/
/*****************/

#define TOKEN_EMPTY   0
#define TOKEN_IDENT   1 /* identifier */
#define TOKEN_STRING  3 /* string */
#define TOKEN_WEIRD   4 /* weird */
#define TOKEN_NUMBER  5 /* number */
#define TOKEN_EOS    99 /* end of string */


/** UTILITY: IDENTIFY A CHARACTER
 * \return 1 if the specified character is a identifier-character
 */
static int is_ident_char(char c) {
//	if ((c>='a') && (c<='z')) return 1;
//	if ((c>='A') && (c<='Z')) return 1;
//	if ((c>='0') && (c<='9')) return 1;
//	if (c=='_') return 1;
//	return 0;
	if ((c==0) || (c=='"') || (c==' ') || (c=='\t')) return 0;
	return 1;
}

/*** UTILITY: IDENTIFY TOKEN TYPE
 * \return type of token at the given string offset
 */
static int token_type(char *s,int offset) {

	/* check if first token character is a 'special' character */
	switch (s[offset]) {
		case 0:
			return TOKEN_EOS;
		case '"':
			return TOKEN_STRING;
		case ' ':
		case '\t':
			return TOKEN_EMPTY;
	}
	
	/* check if first character is a identifier-character */
//	if (is_ident_char(s[offset])) return TOKEN_IDENT;
	return TOKEN_IDENT;
//	return TOKEN_WEIRD;
}


static int ident_size(char *s) {
	int result=1;
	s++;
	while (is_ident_char(*(s++))) result++;
	return result;
}


static int string_size(char *s) {
	int result=1;
	s++;
	while (((*s) != 0) && (*s != '"')) {
		if (*s == '\\') {
			if (*(s+1)=='"') {
				s+=2;
				result+=2;
				continue;
			}
		}
		s++;
		result++;
	}
	if ((*s) == 0) *(s+1)=0;
	return result+1;
}


static int token_size(char *s,int offset) {
	switch (token_type(s,offset)) {
		case TOKEN_IDENT:	return ident_size(s+offset);
		case TOKEN_STRING:	return string_size(s+offset);
		default:			return 1;
	}
}


static int skip_space(char *s,int offset) {
	while ((s[offset] == ' ') || (s[offset] == '\t')) offset++;
	return offset;
}


static int tokenize(char *s,int max_tok,int *offbuf,int *lenbuf) {
	int num_tok=0;
	int offset=0;
	
	/* go to first token of the string */
	while ((*(s+offset))!=0) {
		offset = skip_space(s,offset);
		*offbuf = offset;
		*lenbuf = token_size(s,offset);
		offset += *lenbuf;
		lenbuf++;
		offbuf++;
		num_tok++;
	}
	
	return num_tok;
}


/*** ABSORB EXIT CALLS OF CALLED COMMANDS ***/
static jmp_buf exit_buf;
void flipscmd_exit(int status);
void flipscmd_exit(int status) { longjmp(exit_buf, 0); }
//int sigprocmask(int how, const sigset_t *set, sigset_t *oldset) { return 0; }

/*** MAKE LIFE A BIT EASIER (DEBUGGING) ***/
#define HISTORY_0  "ifconfig eth0 192.168.0.1 255.255.255.0"
#define HISTORY_1  "ifconfig lo 127.0.0.1  255.255.255.0"
#define HISTORY_2  "httpstart"
#define HISTORY_3  "httpdump"

/*** MAIN PROGRAM ***/
int main(int argc, char **argv) {

	char hist[HISTORY_SIZE];
	char dst[MAX_CMD_LEN+10];
	int  tok_off[MAX_TOKENS];
	int  tok_len[MAX_TOKENS];
	struct flips_cmd_struct *cmd;
	int i;
	int cmd_argc;
	char *cmd_argv[MAX_TOKENS];
	
	struct history *history;
	
	term_init("Flips-Terminal");
	history = term_history_create(&hist[0], HISTORY_SIZE);

	term_history_add(history, HISTORY_0);
	term_history_add(history, HISTORY_1);
	term_history_add(history, HISTORY_2);
	term_history_add(history, HISTORY_3);

	do {
		/* read command line */
		term_printf("%c[33m%c[41mFlips%c[37m>%c[40m",27,27,27,27);
		term_readline(&dst[0], MAX_CMD_LEN, history);

		/* parse string */
		cmd_argc = tokenize(&dst[0], MAX_TOKENS, tok_off, tok_len);
		
		/* fill argument list */
		cmd_argv[0] = "<empty command>";
		for (i=0; i<cmd_argc; i++) {
			if (token_type(&dst[0],tok_off[i]) == TOKEN_STRING) {
				dst[tok_off[i] + tok_len[i] - 1] = 0;
				cmd_argv[i] = &dst[tok_off[i] + 1];
			} else {
				dst[tok_off[i] + tok_len[i]] = 0;
				cmd_argv[i] = &dst[tok_off[i]];
			}
		}
		cmd_argv[cmd_argc] = NULL;
	
		/* find matching command */
		cmd = find_cmd(cmd_argv[0]);
		if (!cmd && cmd_argc) term_printf("ERROR: command %s does not exist.\n", cmd_argv[0]);
		
		/* call command */
		if (cmd && cmd->call && (!setjmp(exit_buf))) {
			cmd->call(cmd_argc,cmd_argv);
			term_printf("\nprogram happily finished main function\n");
		} else {
			term_printf("\nprogram finished via exit call\n");
		}
		
	} while (!streq("exit",cmd_argv[0]));
	term_deinit();
	return 0;
}

