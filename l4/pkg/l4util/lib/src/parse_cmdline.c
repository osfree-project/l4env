/*
 * Parse the command-line for specified arguments and store the values into 
 * variables.
 *
 * For a more detailed documentation, see parse_cmd.h in the include dir.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <alloca.h>
#include <l4/util/getopt.h> 
#include <l4/util/parse_cmd.h>

struct parse_cmdline_struct{
    enum parse_cmd_type type;		// which type (int, switch, string)
    char		shortform;	// short symbol
    const char		*longform;	// long name
    void		*argptr;	// ptr to variable getting the value
    union{
    	int		switch_to;	// value a switch sets
    	const char*	default_string;	// default string value
    	unsigned	default_int;	// default int value
    }u;
    const char		*comment;	// a description for the generated help
};
	
#define TRASH(type, val) { type dummy __attribute__ ((unused)) = (val); }

int parse_cmdline(int *argc, const char***argv, char arg0,...){
    va_list va;
    int c, count, shortform, cur_longopt;
    const char*longform, *comment;
    struct option *longopts, *longptr;
    char *optstring, *optptr;
    struct parse_cmdline_struct *pa;
    int err;

    /* calculate the number of argument-descriptors */
    va_start(va, arg0);
    shortform=arg0;
    for(count=0; shortform; count++){
	int type;
	int standard_int, *int_p;
	const char *standard_string, **string_p;
 
 	longform = va_arg(va, const char*);
	comment = va_arg(va, const char*);
  	type = va_arg(va, int);
  	switch(type){
	case PARSE_CMD_INT:
	    standard_int = va_arg(va, int);
	    int_p = va_arg(va, int*);
	    *int_p = standard_int;
	    break;
	case PARSE_CMD_SWITCH:
	    TRASH(int, va_arg(va, int));
	    TRASH(int*, va_arg(va, int*));
	    break;
	case PARSE_CMD_STRING:
	    standard_string = va_arg(va, char*);
	    string_p  = va_arg(va, const char**);
	    *string_p = standard_string;
	    break;
	default:
	    return -1;
  	}
  	shortform = va_arg(va, int);
    }
    va_end(va);

    /* consider the --help and -h */
    count++;

    /* allocate the fields for short options, long options and parse args */
    longopts = (struct option*)alloca(sizeof(struct option)*(count+1));
    if(longopts==0) return -2;
    
    optstring = (char*)alloca(count*2+1);
    if(optstring==0) return -2;
    
    pa = (struct parse_cmdline_struct*)
    	 alloca(count * sizeof(struct parse_cmdline_struct));
    if(pa==0) return -2;
    
    /* fill in the short options field, longopts and parse args */
    va_start(va, arg0);
    shortform = arg0;
    optptr    = optstring;
    longptr   = longopts;

    /* Prefill the 'help' switches. We know it is the first entry, so
       we can check for idx 0 when parsing the table. */
    *optptr++='h';
    pa->shortform = 'h';
    pa->longform = "help";
    pa->comment = "this help";
    pa->type = PARSE_CMD_SWITCH;
    longptr->name = pa->longform;
    longptr->flag = &cur_longopt;
    longptr->val = 0;
    longptr->has_arg = 0;
    longptr++;

    for(c=1;shortform; c++){
	if(shortform!=' ') *optptr++ = shortform;
	pa[c].shortform = shortform;
	pa[c].longform = va_arg(va, const char*);
	pa[c].comment = va_arg(va, const char*);
	pa[c].type = va_arg(va, int);

	/* prefill a few of the longoptions fields */
	if(pa[c].longform){
	    longptr->name = pa[c].longform;
	    longptr->flag = &cur_longopt;
	    longptr->val = c;
	}
	switch(pa[c].type){
	case PARSE_CMD_INT:
	    if(shortform!=' ') *optptr++ = ':';
	    if(pa[c].longform) longptr->has_arg = 1;

	    pa[c].u.default_int = va_arg(va, int);
	    pa[c].argptr = va_arg(va, int*);
	    break;

	case PARSE_CMD_SWITCH:
	    if(pa[c].longform) longptr->has_arg = 0;

	    pa[c].u.switch_to = va_arg(va, int);
	    pa[c].argptr = va_arg(va, int*);
	    break;
	case PARSE_CMD_STRING:
	    if(shortform!=' ') *optptr++ = ':';
	    if(pa[c].longform) longptr->has_arg = 1;

	    pa[c].u.default_string = va_arg(va, char*);
	    pa[c].argptr = va_arg(va, char**);
	    break;
  	}
	if(pa[c].longform) longptr++;
	// next short form
  	shortform = va_arg(va, int);
    }

    // end the optstring string
    *optptr=0;

    // end the longopt field
    longptr->name=0;
    longptr->has_arg=0;
    longptr->flag=0;
    longptr->val=0;

    va_end(va);

    err = -3;
    
    /* now, parse the arguments */
    do{
	int val;
	int idx;

	val = getopt_long_only(*argc, (char**)*argv, optstring, longopts, &idx);
	switch(val){
	case ':':
	case '?':
	    goto e_help;
	case -1:
	    *argc-=optind;
	    *argv+=optind;
	    return 0;
	default:
	    /* we got an option. If it is a short option (val!=0),
	       lookup the index. */
	    if(val!=0){
		for(idx = 0; idx < count; idx++){
		    if(pa[idx].shortform == val) break;
		}
	    } else {
		/* it was a long option. We are lucky, the pa-element is
		   stored in the cur_longopt variable. */
		idx = cur_longopt;
	    }
	    if(idx == 0){
		err = -4;
		goto e_help;
	    }
	    if(idx<count){
		switch(pa[idx].type){
		case PARSE_CMD_INT:
		    *((int*)pa[idx].argptr) = strtol(optarg, 0, 0);
		    break;
		case PARSE_CMD_SWITCH:
		    *((int*)pa[idx].argptr) = pa[idx].u.switch_to;
		    break;
		case PARSE_CMD_STRING:
		    *((const char**)pa[idx].argptr) = optarg;
		    break;
		}
		break;
	    }
	    break;
	} // switch val
    } while(1);

  e_help:
    printf("Usage: %s <options>. Option list:\n", *argv[0]);
    for(c=0;c<count;c++){
	int l;
	char buf[3];
	
	if(pa[c].shortform!=' '){
		buf[0]='-';buf[1]=pa[c].shortform;buf[2]=0;
	} else {
		buf[0]=0;
	}
	
	l = printf(" [ %s%s%s%s%s ]",
		   buf,
		   (buf[0] && pa[c].longform) ? " | " : "",
		   pa[c].longform ? "--" : "",
		   pa[c].longform ? pa[c].longform : "",
		   pa[c].type==PARSE_CMD_INT ? " num" :
		   pa[c].type==PARSE_CMD_STRING ? " string" : "");
	if(pa[c].comment) printf(" %*s- %s", l<25?25-l:0,
				 "", pa[c].comment);
	if(pa[c].type == PARSE_CMD_STRING)
		printf(" (\"%s\")", pa[c].u.default_string);
	if(pa[c].type == PARSE_CMD_INT)
		printf(" (%#x)", pa[c].u.default_int);
	printf("\n");
    }

  return err;
}

