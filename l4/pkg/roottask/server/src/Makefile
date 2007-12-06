SYSTEMS         := x86-l4v2 arm-l4v2 amd64-l4v2 x86-l4secv2emu
PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= roottask
MODE		= sigma0

DEPENDS_PKGS	= demangle

PRIVATE_INCDIR	= $(PKGDIR)/server/src .

DEFAULT_RELOC_x86	:= 0x00110000
DEFAULT_RELOC_amd64	:= 0x00110000
DEFAULT_RELOC_arm 	:= 0x00070000

SRC_C_x86	= iomap.c
SRC_C_amd64	= iomap.c
SRC_C_TASKLIB-y = l4ts_if.c
SRC_C		= rmgr.c quota.c names.c \
		  memmap.c memmap_lock.c region.c irq.c small.c \
		  vm.c pager.c task.c irq_thread.c \
		  misc.c $(SRC_C_TASKLIB-$(USE_TASKLIB)) $(SRC_C_$(ARCH)) \
		  libc_support.c
SRC_C_root_inits.o  = init.c lines.c symbols.c cfg-parse.c exec.c bootquota.c \
		  boot_error.c module.c
SRC_CC		= kinfo.cc
SRC_S		= crt0.S __memmap.S __pool.S
OBJS		= root_inits.o trampoline.s.o
CRT0 		=
LDSCRIPT	= roottask.ld
WARNINGS_cfg-parse.o = -Wno-unused -Wno-parentheses \
		       -Wno-missing-prototypes -Wno-missing-declarations

include $(L4DIR)/mk/prog.mk


LIBCLIBS        = -nostdlib $(GCCLDNOSTDLIB)                    \
		  -luc_c                                        \
		  -luclibc_support                              \
		  -luc_be_minimal_io                            \
		  -luc_be_l4_start_stop                         \
		  -luc_c -ll4util				\
		  -luc_c $(GCCLIB) -luc_c -llist_alloc

L4LIBS          = -lsigma0 -ll4sys

SERVERIDL	= rmgr.idl

ifeq ($(USE_TASKLIB),y)
vpath generic_ts-server.c $(OBJ_BASE)/pkg/task/idl/OBJ-$(ARCH)-$(L4API)
SERVERIDL	+= generic_ts.idl
endif

LDFLAGS := $(filter-out -Ttext, $(LDFLAGS))

cfg-parse.c: cfg-scan.c

$(TARGET): roottask.ld

vpath roottask.ld.in $(PKGDIR)/server/src

roottask.ld: $(SRC_DIR)/roottask.ld.in $(SRC_DIR)/Makefile
	@$(GEN_MESSAGE)
	$(VERBOSE)sed 's,@linkaddr@,$(call default_reloc,$(TARGET)),g' $< > $@;

OBJS_root_inits.o = $(patsubst %.c,%.o,$(SRC_C_root_inits.o))

$(OBJS_root_inits.o): .general.d

-include $(foreach file,$(OBJS_root_inits.o),$(dir $(file)).$(notdir $(file)).d)

root_inits.o: $(OBJS_root_inits.o)
	@$(LINK_MESSAGE)
	$(VERBOSE)$(LD) -m $(LD_EMULATION) -r -o $@ \
	  $(OBJS_root_inits.o) $(addprefix -L,$(L4LIBDIR)) -ldemangle

