#include <l4/env/init.h>
#include <l4/log/l4log.h>

char LOG_tag[9]="initcall";

static void init1(void) {
	LOG_Enter();
}

static void init2(void) {
	LOG_Enter();
}

l4env_initcall(init1);
l4env_initcall(init2);

int main(int argc, char**argv){
	LOG("Here I am, calling l4env_do_initcalls() now...");
	l4env_do_initcalls();
	LOG("Back we are. Finishing...");
	return 0;
}
