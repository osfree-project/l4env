dnl  Alpha ev6 mpn_add_n -- Add two limb vectors of the same length > 0 and
dnl  store sum in a third limb vector.

dnl  Copyright 2000 Free Software Foundation, Inc.

dnl  This file is part of the GNU MP Library.

dnl  The GNU MP Library is free software; you can redistribute it and/or modify
dnl  it under the terms of the GNU Lesser General Public License as published
dnl  by the Free Software Foundation; either version 2.1 of the License, or (at
dnl  your option) any later version.

dnl  The GNU MP Library is distributed in the hope that it will be useful, but
dnl  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
dnl  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
dnl  License for more details.

dnl  You should have received a copy of the GNU Lesser General Public License
dnl  along with the GNU MP Library; see the file COPYING.LIB.  If not, write to
dnl  the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
dnl  MA 02111-1307, USA.

include(`../config.m4')

dnl  INPUT PARAMETERS
dnl  res_ptr	r16
dnl  s1_ptr	r17
dnl  s2_ptr	r18
dnl  size	r19

dnl  This code runs at 5.4 cycles/limb on EV5, and 2.1 cycles/limb on EV6.

dnl This code was written in close cooperation with ev6 pipeline expert
dnl Steve Root.  Any errors are tege's fault, though.

dnl  work triplet  0-2
dnl  work triplet  3-5
dnl  work triplet  6-8
dnl  work triplet  9-11
dnl  carry's 20-23

dnl  sustains 8 adds in 17 cycles !
dnl   (from the d_cache)

dnl  pair loads and stores where possible
dnl  store pairs oct-aligned where possible
dnl    (didn't need it here)
dnl  stores are delayed every third cycle
dnl  loads and stores are delayed by fills
dnl  U stays still, put code there where possible
dnl   (note alternation of U1 and U0)
dnl  L moves because of loads and stores
dnl  note dampers in L to limit damage
dnl  note, load ahead of time where possible

dnl  this odd-looking optimization expects
dnl  that were having random bits in our data, so
dnl  that a pure zero result is unlikely. so we
dnl  penalize the unlikely case to help the
dnl  common case.

ASM_START()
PROLOGUE(mpn_add_n)
	lda	r30,	-240(r30)
	stq	r9,	8(r30)
	stq	r10,	16(r30)
	stq	r11,	24(r30)

	lda	r19,	-8(r19)		C L1 move counter

	bis	r31,	r31,	r23
	blt	r19,	$Lsmall

	ldq	r0,	0(r17)		C L0 get next ones
	ldq	r1,	0(r18)		C L1
	ldq	r3,	8(r17)		C L0 get next ones
	ldq	r4,	8(r18)		C L1
	ldq	r6,	16(r17)		C L0 get next ones
	ldq	r7,	16(r18)		C L1

	ldq	r9,	24(r17)		C L0 get next ones
	ldq	r10,	24(r18)		C L1

	addq	r0,	r1,	r2	C U1 add two data

	cmpult	r2,	r1,	r20	C U1 did it carry

	ldq	r0,	32(r17)		C L0 get next ones
	ldq	r1,	32(r18)		C L1

	addq	r3,	r4,	r5	C U0 add two data

	cmpult	r5,	r4,	r21	C U0 did it carry
	ldq	r3,	40(r17)		C L0 get next ones
	ldq	r4,	40(r18)		C L1

	addq	r6,	r7,	r8	C U1 add two data
	addq	r5,	r20,	r5	C U0 carry from last
	stq	r2,	0(r16)		C L1

	cmpult	r8,	r7,	r22	C U1 did it carry
	beq	r5,	$fix5w		C U0 fix exact zero
$ret5w:	ldq	r6,	48(r17)		C L0 get next ones
	ldq	r7,	48(r18)		C L1

	bis	r31,	r31,	r31	C L  damp out
	addq	r8,	r21,	r8	C U1 carry from last
	bis	r31,	r31,	r31	C L  moves in L !
	addq	r9,	r10,	r11	C U0 add two data

	beq	r8,	$fix6w		C U1 fix exact zero
$ret6w:	cmpult	r11,	r10,	r23	C U0 did it carry
	ldq	r9,	56(r17)		C L0 get next ones
	ldq	r10,	56(r18)		C L1

	lda	r17,	64(r17)		C L0 move pointer
	bis	r31,	r31,	r31	C U
	lda	r18,	64(r18)		C L1 move pointer

	lda	r19,	-8(r19)		C L1 move counter
	blt	r19,	$Lend

C Main loop.  8-way unrolled.
	ALIGN(8)
$Loop:
	addq	r0,	r1,	r2	C U1 add two data
	addq	r11,	r22,	r11	C U0 add in carry
	stq	r5,	8(r16)		C L0 put an answer
	stq	r8,	16(r16)		C L1 pair

	cmpult	r2,	r1,	r20	C U1 did it carry
	beq	r11,	$fix7		C U0 fix exact 0
$ret7:	ldq	r0,	0(r17)		C L0 get next ones
	ldq	r1,	0(r18)		C L1

	bis	r31,	r31,	r31	C L  damp out
	addq	r2,	r23,	r2	C U1 carry from last
	bis	r31,	r31,	r31	C L  moves in L !
	addq	r3,	r4,	r5	C U0 add two data

	beq	r2,	$fix0		C U1 fix exact zero
$ret0:	cmpult	r5,	r4,	r21	C U0 did it carry
	ldq	r3,	8(r17)		C L0 get next ones
	ldq	r4,	8(r18)		C L1

	addq	r6,	r7,	r8	C U1 add two data
	addq	r5,	r20,	r5	C U0 carry from last
	stq	r11,	24(r16)		C L0 store pair
	stq	r2,	32(r16)		C L1

	cmpult	r8,	r7,	r22	C U1 did it carry
	beq	r5,	$fix1		C U0 fix exact zero
$ret1:	ldq	r6,	16(r17)		C L0 get next ones
	ldq	r7,	16(r18)		C L1

	lda	r16,	64(r16)		C L0 move pointer
	addq	r8,	r21,	r8	C U1 carry from last
	lda	r19,	-8(r19)		C L1 move counter
	addq	r9,	r10,	r11	C U0 add two data

	beq	r8,	$fix2		C U1 fix exact zero
$ret2:	cmpult	r11,	r10,	r23	C U0 did it carry
	ldq	r9,	24(r17)		C L0 get next ones
	ldq	r10,	24(r18)		C L1

	addq	r0,	r1,	r2	C U1 add two data
	addq	r11,	r22,	r11	C U0 add in carry
	stq	r5,	-24(r16)	C L0 put an answer
	stq	r8,	-16(r16)	C L1 pair

	cmpult	r2,	r1,	r20	C U1 did it carry
	beq	r11,	$fix3		C U0 fix exact 0
$ret3:	ldq	r0,	32(r17)		C L0 get next ones
	ldq	r1,	32(r18)		C L1

	bis	r31,	r31,	r31	C L  damp out
	addq	r2,	r23,	r2	C U1 carry from last
	bis	r31,	r31,	r31	C L  moves in L !
	addq	r3,	r4,	r5	C U0 add two data

	beq	r2,	$fix4		C U1 fix exact zero
$ret4:	cmpult	r5,	r4,	r21	C U0 did it carry
	ldq	r3,	40(r17)		C L0 get next ones
	ldq	r4,	40(r18)		C L1

	addq	r6,	r7,	r8	C U1 add two data
	addq	r5,	r20,	r5	C U0 carry from last
	stq	r11,	-8(r16)		C L0 store pair
	stq	r2,	0(r16)		C L1

	cmpult	r8,	r7,	r22	C U1 did it carry
	beq	r5,	$fix5		C U0 fix exact zero
$ret5:	ldq	r6,	48(r17)		C L0 get next ones
	ldq	r7,	48(r18)		C L1

	bis	r31,	r31,	r31	C L  damp out
	addq	r8,	r21,	r8	C U1 carry from last
	bis	r31,	r31,	r31	C L  moves in L !
	addq	r9,	r10,	r11	C U0 add two data

	beq	r8,	$fix6		C U1 fix exact zero
$ret6:	cmpult	r11,	r10,	r23	C U0 did it carry
	ldq	r9,	56(r17)		C L0 get next ones
	ldq	r10,	56(r18)		C L1

	lda	r17,	64(r17)		C L0 move pointer
	bis	r31,	r31,	r31	C U
	lda	r18,	64(r18)		C L1 move pointer
	bge	r19,	$Loop		C U1 loop control
C ==== main loop end

$Lend:
	addq	r0,	r1,	r2	C U1 add two data
	addq	r11,	r22,	r11	C U0 add in carry
	stq	r5,	8(r16)		C L0 put an answer
	stq	r8,	16(r16)		C L1 pair

	cmpult	r2,	r1,	r20	C U1 did it carry
	beq	r11,	$fix7c		C U0 fix exact 0
$ret7c:
	addq	r2,	r23,	r2	C U1 carry from last
	addq	r3,	r4,	r5	C U0 add two data

	beq	r2,	$fix0c		C U1 fix exact zero
$ret0c:	cmpult	r5,	r4,	r21	C U0 did it carry

	addq	r6,	r7,	r8	C U1 add two data
	addq	r5,	r20,	r5	C U0 carry from last
	stq	r11,	24(r16)		C L0 store pair
	stq	r2,	32(r16)		C L1

	cmpult	r8,	r7,	r22	C U1 did it carry
	beq	r5,	$fix1c		C U0 fix exact zero
$ret1c:
	lda	r16,	64(r16)		C L0 move pointer
	addq	r8,	r21,	r8	C U1 carry from last
	addq	r9,	r10,	r11	C U0 add two data

	beq	r8,	$fix2c		C U1 fix exact zero
$ret2c:	cmpult	r11,	r10,	r23	C U0 did it carry

	addq	r11,	r22,	r11	C U0 add in carry
	stq	r5,	-24(r16)	C L0 put an answer
	stq	r8,	-16(r16)	C L1 pair

	beq	r11,	$fix3c		C U0 fix exact 0
$ret3c:
	stq	r11,	-8(r16)		C L0 store pair


$Lsmall:
	lda	r19,	8(r19)
	beq	r19,	$Lret

	ldq	r0,	0(r17)
	ldq	r1,	0(r18)
	lda	r19,	-1(r19)
	beq	r19,	$Lend0

	ALIGN(8)
$Loop0:	addq	r0,	r1,	r2	C main add
	ldq	r0,	8(r17)
	cmpult	r2,	r1,	r8	C compute cy from last add
	ldq	r1,	8(r18)
	addq	r2,	r23,	r20	C carry add
	lda	r17,	8(r17)
	lda	r18,	8(r18)
	stq	r20,	0(r16)
	cmpult	r20,	r2,	r23	C compute cy from last add
	lda	r19,	-1(r19)		C decr loop cnt
	bis	r8,	r23,	r23	C combine cy from the two adds
	lda	r16,	8(r16)
	bne	r19,	$Loop0
$Lend0:	addq	r0,	r1,	r2	C main add
	addq	r2,	r23,	r20	C carry add
	cmpult	r2,	r1,	r8	C compute cy from last add
	cmpult	r20,	r2,	r23	C compute cy from last add
	stq	r20,	0(r16)
	bis	r8,	r23,	r23	C combine cy from the two adds

$Lret:
	lda	r0,	0(r23)		C copy carry into return register

	ldq	r9,	8(r30)
	ldq	r10,	16(r30)
	ldq	r11,	24(r30)
	lda	r30,	240(r30)
	ret	r31,(r26),1


$fix5w:	bis	r21,	r20,	r21	C bring forward carry
	br	r31,	$ret5w
$fix6w:	bis	r22,	r21,	r22	C bring forward carry
	br	r31,	$ret6w
$fix0:	bis	r20,	r23,	r20	C bring forward carry
	br	r31,	$ret0
$fix1:	bis	r21,	r20,	r21	C bring forward carry
	br	r31,	$ret1
$fix2:	bis	r22,	r21,	r22	C bring forward carry
	br	r31,	$ret2
$fix3:	bis	r23,	r22,	r23	C bring forward carry
	br	r31,	$ret3
$fix4:	bis	r20,	r23,	r20	C bring forward carry
	br	r31,	$ret4
$fix5:	bis	r20,	r21,	r21	C bring forward carry
	br	r31,	$ret5
$fix6:	bis	r22,	r21,	r22	C bring forward carry
	br	r31,	$ret6
$fix7:	bis	r23,	r22,	r23	C bring forward carry
	br	r31,	$ret7
$fix0c:	bis	r20,	r23,	r20	C bring forward carry
	br	r31,	$ret0c
$fix1c:	bis	r21,	r20,	r21	C bring forward carry
	br	r31,	$ret1c
$fix2c:	bis	r22,	r21,	r22	C bring forward carry
	br	r31,	$ret2c
$fix3c:	bis	r23,	r22,	r23	C bring forward carry
	br	r31,	$ret3c
$fix7c:	bis	r23,	r22,	r23	C bring forward carry
	br	r31,	$ret7c

EPILOGUE(mpn_add_n)
ASM_END()
