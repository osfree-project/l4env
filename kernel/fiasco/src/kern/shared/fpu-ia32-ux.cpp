/*
 * Fiasco FPU Code
 * Shared between UX and native IA32.
 */

INTERFACE:

EXTENSION class Fpu
{

private:
  struct fpu_regs       // saved FPU registers
  {
    long    cwd;
    long    swd;
    long    twd;
    long    fip;
    long    fcs;
    long    foo;
    long    fos;
    long    st_space[20];   /* 8*10 bytes for each FP-reg = 80 bytes */
  };

  struct sse_regs 
  {
    unsigned short  cwd;
    unsigned short  swd;
    unsigned short  twd;
    unsigned short  fop;
    long    fip;
    long    fcs;
    long    foo;
    long    fos;
    long    mxcsr;
    long    reserved;
    long    st_space[32];   /* 8*16 bytes for each FP-reg = 128 bytes */
    long    xmm_space[32];  /* 8*16 bytes for each XMM-reg = 128 bytes */
    long    padding[56];
  };
};

IMPLEMENTATION[ia32-ux]:

#include <cstring>
#include "cpu.h"
#include "fpu_state.h"
#include "regdefs.h"

/*
 * Initialize FPU or SSE state
 * We don't use finit, because it is slow. Initializing the context in
 * memory and fetching it via restore_state is supposedly faster
 */
IMPLEMENT inline NEEDS ["cpu.h", "fpu_state.h", "regdefs.h", <cstring>]
void
Fpu::init_state (Fpu_state *s)
{
  if ((Cpu::features() & FEAT_FXSR)) {

    sse_regs *sse = reinterpret_cast<sse_regs *>(s->state_buffer());
    
    memset (sse, 0, sizeof (*sse));
    sse->cwd = 0x37f;
    
    if ((Cpu::features() & FEAT_SSE))
      sse->mxcsr = 0x1f80;
    
  } else {
    
    fpu_regs *fpu = reinterpret_cast<fpu_regs *>(s->state_buffer());
    
    memset (fpu, 0, sizeof (*fpu));
    fpu->cwd = 0xffff037f;
    fpu->swd = 0xffff0000;
    fpu->twd = 0xffffffff;
    fpu->fos = 0xffff0000;
  }
}

/** 
 * Return size of FPU context structure, depending on i387 or SSE
 * @return size of FPU context structure
 */
IMPLEMENT inline NEEDS ["cpu.h", "regdefs.h"]
unsigned const
Fpu::state_size()
{
  return (Cpu::features() & FEAT_FXSR) ? sizeof (sse_regs) :
                                         sizeof (fpu_regs);
}

/**
 * Return recommended FPU context alignment, depending on i387 or SSE
 * @return recommended FPU context alignment
 */
IMPLEMENT inline NEEDS ["cpu.h", "regdefs.h"]
unsigned const
Fpu::state_align()
{
  return (Cpu::features() & FEAT_FXSR) ? 16 : 4;
}
