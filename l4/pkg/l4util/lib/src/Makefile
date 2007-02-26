# Makefile for the sources of the l4util library

PKGDIR	?= ../..
L4DIR	?= $(PKGDIR)/../..

SYSTEMS	= x86 x86-l4v2 x86-l4x0

SRC_C_x86-l4v2	= apic.c ctype.c getopt.c getopt1.c getopt2.c micros2l4to.c \
		  perform.c queue.c rand.c rdtsc.c sleep.c \
		  spin.c thread.c vsprintf.c wait_queue.c \
		  base64.c
SRC_C_x86-l4x0	= apic.c ctype.c getopt.c getopt1.c getopt2.c micros2l4to.c \
		  perform.c queue.c rand.c rdtsc.c sleep.c \
		  spin.c thread.c vsprintf.c wait_queue.c \
		  base64.c
SRC_C_libparsecmdline.a	= parse_cmdline.c base64.c
SRC_C_libl4util.a_x86 = rdtsc.c

TARGET		=
TARGET_x86	= libparsecmdline.a lib$(PKGNAME).a
TARGET_x86-l4v2	= lib$(PKGNAME).a
TARGET_x86-l4x0	= lib$(PKGNAME).a
MODE		= sigma0

PRIVATE_INCDIR_perform.o = .
PRIVATE_INCDIR_perform.s.o = .

all::
perform.o:	pmc_events.h
pmc_events.h: pmc_events.in
	sort < $^ > $^.sorted || rm $^.sorted
	echo "/* created automatically from $^, do not edit */" > $@
	awk '/^[0-9a-fA-F][0-9a-fA-F].*/{print "{0x"toupper($$1)", \""$$2"\"},"}' \
             < $^.sorted >> $@ || rm $@
	rm $^.sorted

vpath pmc_events.in ..

include $(L4DIR)/mk/lib.mk
