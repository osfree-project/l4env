/**
 * \file   sincos.c
 * \brief  Provide a sincos and sincosf implementation, used implicitly by gcc
 *
 * \date   2008-05-27
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 */
/* (c) 2008 Technische Universität Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifdef ARCH_arm
#define JUMP_INSN "b"
#else
#define JUMP_INSN "jmp"
#endif

asm("l4libc_sin : " JUMP_INSN " sin \n");
asm("l4libc_cos : " JUMP_INSN " cos \n");
asm("l4libc_sinf: " JUMP_INSN " sinf\n");
asm("l4libc_cosf: " JUMP_INSN " cosf\n");

double l4libc_sin(double x);
double l4libc_cos(double x);
float  l4libc_sinf(float x);
float  l4libc_cosf(float x);

void sincos(double x, double *s, double *c);
void sincosf(float x, float *s, float *c);

void sincos(double x, double *s, double *c)
{
  *s = l4libc_sin(x);
  *c = l4libc_cos(x);
}

void sincosf(float x, float *s, float *c)
{
  *s = l4libc_sinf(x);
  *c = l4libc_cosf(x);
}
