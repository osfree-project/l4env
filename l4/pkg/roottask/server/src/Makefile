SYSTEMS         := x86-l4v2 arm-l4x0 amd64-l4v2 x86-l4secv2emu
PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= roottask
MODE		= sigma0

PRIVATE_INCDIR	= $(PKGDIR)/server/src .

DEFAULT_RELOC_x86	:= 0x00120000
DEFAULT_RELOC_arm 	:= 0x00060000
DEFAULT_RELOC_amd64	:= 0x00120000

SRC_C_x86	= iomap.c
SRC_C_amd64	= iomap.c
SRC_C		= rmgr.c exec.c init.c module.c quota.c names.c \
		  memmap.c memmap_lock.c region.c irq.c small.c symbols.c \
		  vm.c lines.c boot_error.c pager.c task.c irq_thread.c \
		  bootquota.c cfg-parse.c misc.c $(SRC_C_$(ARCH)) \
		  libc_support.c
SRC_CC		= kinfo.cc
SRC_S		= crt0.S __memmap.S __pool.S
OBJS		= trampoline.s.o
CRT0 		=
LDSCRIPT	= roottask.ld
WARNINGS_cfg-parse.o = -Wno-unused -Wno-parentheses \
		       -Wno-missing-prototypes -Wno-missing-declarations
SERVERIDL	= rmgr.idl

include $(L4DIR)/mk/prog.mk


ifeq ($(USE_DIETLIBC),y)
LIBCLIBS        = -nostdlib $(GCCLDNOSTDLIB)                    \
		  -ldiet_c                                      \
		  -ldietlibc_support                            \
		  -ldiet_be_minimal_io                          \
		  -ldiet_be_l4_start_stop                       \
		  -ldiet_c -ll4util				\
		  -ldiet_c $(GCCLIB) -ldiet_c -llist_alloc
else
LIBCLIBS        = -nostdlib $(GCCLDNOSTDLIB)                    \
		  -luc_c                                        \
		  -luclibc_support                              \
		  -luc_be_minimal_io                            \
		  -luc_be_l4_start_stop                         \
		  -luc_c -ll4util				\
		  -luc_c $(GCCLIB) -luc_c -llist_alloc
endif
L4LIBS          = -ldemangle -lsigma0 -ll4sys

LDFLAGS := $(filter-out -Ttext, $(LDFLAGS))

cfg-parse.c: cfg-scan.c

$(TARGET): roottask.ld

vpath roottask.ld.in $(PKGDIR)/server/src

roottask.ld: roottask.ld.in Makefile
	@$(GEN_MESSAGE)
	$(VERBOSE)sed 's,@linkaddr@,$(call default_reloc,$(TARGET)),g' $< > $@;
