#include <setjmp.h>
//#include <signal.h>

int __sigjmp_save(sigjmp_buf env,int savemask);
int __sigjmp_save(sigjmp_buf env,int savemask) {
/* // Martin: This is removed as we have no signals
  if (savemask) {
    env[0].__mask_was_saved=(sigprocmask(SIG_BLOCK,(sigset_t*)0,(sigset_t*)&env[0].__saved_mask)==0);
  }
*/
  return 0;
}
