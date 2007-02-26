#include <l4/crtx/ctor.h>
#include <l4/log/l4log.h>

char LOG_tag[9]="initcall";

static void __attribute__((constructor)) init1(void) {
	LOG_Enter();
}

static void __attribute__((constructor)) init2(void) {
	LOG_Enter();
}

static void init3(void) {
	LOG_Enter();
}

static void init4(void) {
	LOG_Enter();
}

L4C_CTOR(init3, L4CTOR_AFTER_BACKEND);
L4C_CTOR(init4, L4CTOR_AFTER_BACKEND);

int main(int argc, char**argv){
	LOG("This is the main function...");
	return 0;
}
