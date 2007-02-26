SYSTEMS         := x86-l4v2 x86-l4x0 arm-l4x0
PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= roottask
MODE		= sigma0

PRIVATE_INCDIR	= $(PKGDIR)/server/src .

LINK_ADDR_x86	= 0x00120000
LINK_ADDR_arm_sa  = 0xc0050000
LINK_ADDR_arm_pxa = 0xa0050000
LINK_ADDR_arm	= $(LINK_ADDR_arm_$(CPU))
LINK_ADDR	= $(LINK_ADDR_$(ARCH))

SRC_C_x86	= iomap.c
SRC_C		= rmgr.c exec.c init.c module.c quota.c names.c \
		  memmap.c region.c irq.c small.c symbols.c vm.c \
		  lines.c boot_error.c pager.c task.c irq_thread.c bootquota.c \
		  cfg-parse.c misc.c \
		  $(SRC_C_$(ARCH))
SRC_S		= crt0.S __memmap.S __pool.S
OBJS		= trampoline.s.o
CRT0 		=
LDSCRIPT	= roottask.ld
WARNINGS_cfg-parse.o = -Wno-unused -Wno-parentheses \
		       -Wno-missing-prototypes -Wno-missing-declarations
SERVERIDL	= rmgr.idl

include $(L4DIR)/mk/prog.mk

ifeq ($(USE_DIETLIBC),y)
SRC_C		+= libc_support.c
LIBCLIBS        = -nostdlib $(GCCLDNOSTDLIB)                    \
		  -ldiet_c                                      \
		  -ldietlibc_support                            \
		  -ldiet_be_minimal_io                          \
		  -ldiet_be_l4_start_stop                       \
		  -ldiet_be_sigma0_mem                          \
		  -ldiet_c -ll4util_light			\
		  -ldiet_c $(GCCLIB) -ldiet_c -llist_alloc
else
ifeq ($(USE_UCLIBC),y)
SRC_C		+= libc_support.c
LIBCLIBS        = -nostdlib $(GCCLDNOSTDLIB)                    \
		  -luc_c                                        \
		  -luclibc_support                              \
		  -luc_be_minimal_io                            \
		  -luc_be_l4_start_stop                         \
		  -luc_be_sigma0_mem                            \
		  -luc_c -ll4util_light				\
		  -luc_c $(GCCLIB) -luc_c -llist_alloc
else
SRC_C		+= oskit_support.c
LIBCLIBS	= -nostdlib -lkern -lmc -llmm -ll4util_light $(GCCLIB)
endif
endif
L4LIBS          = -ldemangle -ll4sys

cfg-parse.c: cfg-scan.c

$(TARGET): roottask.ld

vpath roottask.ld.in $(PKGDIR)/server/src

roottask.ld: roottask.ld.in Makefile
	@echo "  ... Making $@"
	$(VERBOSE)sed 's,@linkaddr@,$(LINK_ADDR),g' $< > $@;
