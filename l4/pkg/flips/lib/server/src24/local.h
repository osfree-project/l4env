#include <l4/dde_linux/dde.h>
#define REGISTER_PROCESS_WORKER() l4dde_process_add_worker()
#define REMOVE_PROCESS_WORKER() l4dde_process_remove_worker()
