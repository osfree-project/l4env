#include <stdio.h>
#include <l4/names/libnames.h>
#include <names.h>

int main(int argc, char**argv){
    char name[MAX_NAME_LEN];
    l4_threadid_t id;
    int i;

    for(i=0;i<MAX_ENTRIES; i++){
	if(names_query_nr(i, name, sizeof(name), &id)){
	    printf("%02x.%02x: %s\n",
		   id.id.task, id.id.lthread, name);
	}
    }
    return 0;
}
