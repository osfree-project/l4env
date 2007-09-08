#include <l4/dde/ddekit/initcall.h>

#include <l4/crtx/crt0.h>

void ddekit_do_initcalls() {
	crt0_dde_construction();
}

