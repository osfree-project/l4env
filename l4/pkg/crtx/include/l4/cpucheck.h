#ifndef __CRTX_CPUCHECK_H
#define __CRTX_CPUCHECK_H

#ifndef __ASSEMBLER__
#define CPU_MODEL(model)	".section\t.cpucheck,\"a\",@nobits\n\t"	\
				/*"__cpu_model_" #model ":\n\t"	*/	\
				".comm __cpu_model_" #model ", 4\n\t"	\
				".text\n\t"

#if CPU==486

  asm(CPU_MODEL(4));

#elif CPU==586

  asm(CPU_MODEL(5));

#elif CPU==686
  
  asm(CPU_MODEL(6));

#endif

#undef CPU_SECTION

#endif /* __ASSEMBLER__ */

#endif /* __CRTX_CPUCHECK_H */

