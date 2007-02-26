/*!
 * \file   names/examples/demo/main.c
 * \brief  List all the name/ID pairs registered at names
 *
 * \date   05/27/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <stdio.h>

#include <l4/names/libnames.h>

int main(int argc, char**argv){
    char name[NAMES_MAX_NAME_LEN];
    l4_threadid_t id;
    int i;

    for(i=0;i<NAMES_MAX_ENTRIES; i++){
	if(names_query_nr(i, name, sizeof(name), &id)){
	    printf("%02x.%02x: %s\n",
		   id.id.task, id.id.lthread, name);
	}
    }

    return 0;
}
