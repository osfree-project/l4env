#include <stdio.h>
#include <l4/env/errno.h>
#include <l4/util/util.h>
#include <l4/util/spin.h>
#include <l4/log/l4log.h>
#include <l4/omega0/client.h>
#include <l4/thread/thread.h>

char LOG_tag[9]="irqmeter";

static void thread(void*data){
  omega0_irqdesc_t desc;
  omega0_request_t req;
  int num = (int)data, handle, err;
  char buf[]="x";

  desc.s.shared = 1;
  desc.s.num = num;
  
  if((handle=omega0_attach(desc))<0){
    LOGL("error %d attaching to irq %#X", handle, num-1);
    l4thread_started(NULL);
    l4thread_exit();
  }
  printf("Attached to irq %#X.\n", num-1);
  l4thread_started(NULL);
  
  req = OMEGA0_RQ(OMEGA0_UNMASK, num);
  if((err=omega0_request(handle, req))<0){
    LOGl("Error %d unmasking irq %#X at Omega0\n", err, num-1);
    goto out;
  }
  
  req = OMEGA0_RQ(OMEGA0_WAIT, num);
  sprintf(buf, "%X", num-1);
  while(1){
    if((err=omega0_request(handle, req))<0){
      LOGl("Error %d waiting for next irq (%#X) from Omega0\n", err, num-1);
      goto out;
    }
    l4_spin_n_text_vga((num-1)*3, 0, sizeof(buf)-1, buf);
    //LOG("%s\n",buf);
  }

out:
  /* we had an error, try to detach from omega0 */
  if((err=omega0_detach(desc))<0){
    LOGl("Error %d detaching IRQ %#X from Omega0\n", err, num-1);
  }
  l4thread_exit();
}

static int attach(int num){
  int id;
  
  if((id = l4thread_create(thread, (void*)num, L4THREAD_CREATE_SYNC))<0){
      LOGl("%s\n", l4env_strerror(-id));
      return id;
  }
  return 0;
}

int main(int argc, char*argv[]){
  int i;

  printf("Here I am\n");
  
  for(i=omega0_first();i;i=omega0_next(i)){
  	attach(i);
  }

  printf("Attached to all available irqs, exiting.\n");
  
  while(1)l4_sleep(-1);
}
