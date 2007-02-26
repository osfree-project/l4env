/*!
 * \file   l4util/examples/cmdline/main.c
 * \brief  Example showing the use of cmdline stuff
 *
 * \date   08/23/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/util/parse_cmd.h>
#include <l4/log/l4log.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

char LOG_tag[9]="ex";

static void f1(int id){
    LOG("id=%d", id);
}

static void f2(int id, const char*string, int num){
    LOG("id=%d, string=\"%s\", num=%d", id, string, num);
}


int main(int argc, const char**argv){
    int int_val, counter, switch_val=0;
    char*string_val;

    if(parse_cmdline(&argc, &argv,
		     'i', "integer", "integer argument",
		     PARSE_CMD_INT, 10, &int_val,
		     'x', "switch", "switch",
		     PARSE_CMD_SWITCH, 1, &switch_val,
		     's', "string", "string argument",
		     PARSE_CMD_STRING, "default", &string_val,
		     'f', "func", "function call, no arg",
		     PARSE_CMD_FN, 1, &f1,
		     'F', "argfunc", "function call, with arg",
		     PARSE_CMD_FN_ARG, 2, &f2,
		     'I', "inc", "increment",
		     PARSE_CMD_INC, 0, &counter,
		     'D', "dec", "decrement",
		     PARSE_CMD_DEC, 0, &counter,
		     0, 0)) return 1;
    printf("int=%d switch=%d string=\"%s\" counter=%d\n",
	   int_val, switch_val, string_val, counter);

    return 0;
}
