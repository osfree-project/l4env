# Makefile for the sources of the l4util library

PKGDIR	?= ../..
L4DIR	?= $(PKGDIR)/../..

SYSTEMS		= x86 x86-l4v2 x86-l4x0 arm arm-l4x0

SRC_S_x86	+= ARCH-x86/kip_area.S
SRC_S_arm	+= ARCH-arm/kip_area.S

SRC_C_only_x86	= $(addprefix ARCH-x86/, apic.c perform.c rdtsc.c spin.c)

ALL_SRC_C	= alloc.c getopt2.c micros2l4to.c queue.c rand.c sleep.c \
		  thread.c base64.c slmap.c kip.c reboot.c wait_queue.c \
		  ARCH-$(ARCH)/reboot.c $(SRC_C_only_$(ARCH))

LIGHTFILES	= sleep.c micros2l4to.c ARCH-$(ARCH)/reboot.c


SRC_C_libl4util.a_x86            = alloc.c
SRC_C_libl4util.a_x86-l4v2       = $(ALL_SRC_C) setjmp.c
SRC_C_libl4util.a_x86-l4x0       = $(ALL_SRC_C) setjmp.c
SRC_C_libl4util.a_arm-l4x0       = $(ALL_SRC_C) memdesc.c
SRC_C_libl4util.p.a_x86-l4v2     = $(ALL_SRC_C) setjmp.c
SRC_C_libl4util.p.a_x86-l4x0     = $(ALL_SRC_C) setjmp.c
SRC_C_libl4util.p.a_arm-l4x0     = $(ALL_SRC_C) memdesc.c
SRC_S_libl4util.a_x86-l4v2       = $(SRC_S_$(ARCH))
SRC_S_libl4util.a_x86-l4x0       = $(SRC_S_$(ARCH))
SRC_S_libl4util.a_arm-l4x0       = $(SRC_S_$(ARCH))
SRC_S_libl4util.p.a_x86-l4v2     = $(SRC_S_$(ARCH))
SRC_S_libl4util.p.a_x86-l4x0     = $(SRC_S_$(ARCH))
SRC_S_libl4util.p.a_arm-l4x0     = $(SRC_S_$(ARCH))

SRC_C_libl4util_light.a_x86-l4v2 = $(LIGHTFILES)
SRC_C_libl4util_light.a_x86-l4x0 = $(LIGHTFILES)
SRC_C_libl4util_light.a_arm-l4x0 = $(LIGHTFILES)

SRC_C_libparsecmdline.a	         = parse_cmdline.c base64.c
SRC_C_libmain.a                 += __main.c

SRC_C_liblist_alloc.a            = list_alloc.c

TARGET		=
TARGET_x86	= libparsecmdline.a liblist_alloc.a libl4util.a libmain.a
TARGET_arm	= libparsecmdline.a liblist_alloc.a libl4util.a libmain.a
TARGET_x86-l4v2	= libl4util.a libl4util_light.a
TARGET_x86-l4x0	= libl4util.a libl4util_light.a
TARGET_arm-l4x0	= libl4util.a libl4util_light.a
MODE		= sigma0

PRIVATE_INCDIR_ARCH-x86/perform.o = ARCH-x86
PRIVATE_INCDIR_ARCH-x86/perform.s.o = ARCH-x86
PRIVATE_INCDIR_ARCH-x86/perform.pr.o = ARCH-x86

include $(L4DIR)/mk/lib.mk

# dietlibc has its own getopt
ifeq ($(USE_DIETLIBC)$(USE_UCLIBC),nn)
ALL_SRC_C	+= getopt.c getopt1.c
endif

ARCH-x86/perform.o:	ARCH-x86/pmc_events.h
ARCH-x86/pmc_events.h: pmc_events.in
	@$(GEN_MESSAGE)
	$(VERBOSE)sort < $^ > $^.sorted || rm $^.sorted
	$(VERBOSE)echo "/* created automatically from $^, do not edit */" > $@
	$(VERBOSE)awk '/^[0-9a-fA-F][0-9a-fA-F].*/{print "{0x"toupper($$1)", \""$$2"\"},"}' < $^.sorted >> $@ || rm $@
	$(VERBOSE)rm $^.sorted

vpath pmc_events.in ../ARCH-x86
