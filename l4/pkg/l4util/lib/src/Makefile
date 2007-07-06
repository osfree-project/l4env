# Makefile for the sources of the l4util library

PKGDIR	?= ../..
L4DIR	?= $(PKGDIR)/../..

SYSTEMS               = x86 x86-l4v2 arm arm-l4v2 amd64 amd64-l4v2

ALL_SRC_C_only_x86    = $(addprefix ARCH-x86/, apic.c perform.c spin.c)

ROOT_SRC_C_only_x86   = $(addprefix ARCH-x86/, rdtsc.c)
ROOT_SRC_C_only_amd64 = $(addprefix ARCH-x86/, rdtsc.c)

ALL_SRC_C_NOAPI       = alloc.c getopt2.c rand.c base64.c slmap.c kprintf.c \
			ARCH-$(ARCH)/reboot.c
ALL_SRC_C_L4API       = alloc.c getopt2.c micros2l4to.c queue.c rand.c sleep.c \
                        thread.c base64.c slmap.c memdesc.c \
			kprintf.c \
                        ARCH-$(ARCH)/reboot.c $(ALL_SRC_C_only_$(ARCH))

ROOT_SRC_C            = reboot.c $(ROOT_SRC_C_only_$(ARCH))


SRC_C_libl4util.a_x86            = $(ALL_SRC_C_NOAPI)
SRC_C_libl4util.a_amd64          = $(ALL_SRC_C_NOAPI)
SRC_C_libl4util.a_x86-l4v2       = $(ALL_SRC_C_L4API) ARCH-x86/setjmp.c
SRC_C_libl4util.a_amd64-l4v2     = $(ALL_SRC_C_L4API) ARCH-amd64/setjmp.c
SRC_C_libl4util.a_arm-l4v2       = $(ALL_SRC_C_L4API)
SRC_C_libl4util.p.a_x86-l4v2     = $(ALL_SRC_C_L4API) ARCH-x86/setjmp.c
SRC_C_libl4util.p.a_amd64-l4v2   = $(ALL_SRC_C_L4API) ARCH-amd64/setjmp.c
SRC_C_libl4util.p.a_arm-l4v2     = $(ALL_SRC_C_L4API)
SRC_S_libl4util.a_x86-l4v2       = $(SRC_S_$(ARCH))
SRC_S_libl4util.a_amd64-l4v2     = $(SRC_S_$(ARCH))
SRC_S_libl4util.a_arm-l4v2       = $(SRC_S_$(ARCH))
SRC_S_libl4util.p.a_x86-l4v2     = $(SRC_S_$(ARCH))
SRC_S_libl4util.p.a_amd64-l4v2   = $(SRC_S_$(ARCH))
SRC_S_libl4util.p.a_arm-l4v2     = $(SRC_S_$(ARCH))

SRC_C_libl4util_root.a_x86-l4v2    = $(ROOT_SRC_C)
SRC_C_libl4util_root.a_amd64-l4v2  = $(ROOT_SRC_C)
SRC_C_libl4util_root.a_arm-l4v2    = $(ROOT_SRC_C)
SRC_C_libl4util_root.p.a_x86-l4v2  = $(ROOT_SRC_C)
SRC_C_libl4util_root.p.a_arm-l4v2  = $(ROOT_SRC_C)
SRC_S_libl4util_root.a_x86-l4v2    = $(ROOT_SRC_S_$(ARCH))
SRC_S_libl4util_root.a_arm-l4v2    = $(ROOT_SRC_S_$(ARCH))
SRC_S_libl4util_root.p.a_x86-l4v2  = $(ROOT_SRC_S_$(ARCH))
SRC_S_libl4util_root.p.a_arm-l4v2  = $(ROOT_SRC_S_$(ARCH))
SRC_S_libl4util_root.a_amd64-l4v2  = $(ROOT_SRC_S_$(ARCH))

SRC_C_libparsecmdline.a	         = parse_cmdline.c base64.c
SRC_C_libmain.a                 += __main.c

SRC_C_liblist_alloc.a            = list_alloc.c
SRC_C_liblist_alloc.p.a          = list_alloc.c

TARGET		=
TARGET_x86	= libparsecmdline.a liblist_alloc.a libl4util.a libmain.a
TARGET_arm	= libparsecmdline.a liblist_alloc.a libl4util.a libmain.a
TARGET_amd64	= libparsecmdline.a liblist_alloc.a libl4util.a libmain.a
TARGET_x86-l4v2	= libl4util.a libl4util_root.a
TARGET_arm-l4v2	= libl4util.a libl4util_root.a
TARGET_amd64-l4v2 = libl4util.a libl4util_root.a
MODE		= sigma0
BUILD_PIC	= liblist_alloc.a

PRIVATE_INCDIR_ARCH-x86/perform.o = ARCH-x86
PRIVATE_INCDIR_ARCH-x86/perform.s.o = ARCH-x86
PRIVATE_INCDIR_ARCH-x86/perform.pr.o = ARCH-x86

include $(L4DIR)/mk/lib.mk

ARCH-x86/perform.o:	ARCH-x86/pmc_events.h
ARCH-x86/pmc_events.h: pmc_events.in
	@$(GEN_MESSAGE)
	$(VERBOSE)sort < $^ > $(OBJ_DIR)/$(^F).sorted || rm $(OBJ_DIR)/$(^F).sorted
	$(VERBOSE)echo "/* created automatically from $^, do not edit */" > $@
	$(VERBOSE)awk '/^[0-9a-fA-F][0-9a-fA-F].*/{print "{0x"toupper($$1)", \""$$2"\"},"}' < $(OBJ_DIR)/$(^F).sorted >> $@ || rm $@
	$(VERBOSE)rm $(OBJ_DIR)/$(^F).sorted

vpath pmc_events.in $(SRC_DIR)/ARCH-x86
