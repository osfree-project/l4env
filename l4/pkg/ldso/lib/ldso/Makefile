PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

TARGET		 = libld-l4.s.so
MODE		 = l4env
SYSTEMS		 = x86-l4v2 arm-l4v2 amd64-l4v2
PRIVATE_INCDIR	 = $(PKGDIR)/lib/ldso \
		   $(PKGDIR)/lib/ldso/include \
		   $(PKGDIR)/lib/ldso/include/sysdeps/linux/$(LDSO_ARCH_NAME) \
		   $(PKGDIR)/lib/ldso/include/sysdeps/linux/common \
		   $(PKGDIR)/lib/ldso/include/ldso/$(LDSO_ARCH_NAME) \
		   $(PKGDIR)/lib/ldso/include/ldso
LDSO_ARCH_NAME_x86 = i386
LDSO_ARCH_NAME_amd64 = x86_64
LDSO_ARCH_NAME_arm = arm
LDSO_ARCH_NAME	= $(LDSO_ARCH_NAME_$(ARCH))
DEFINES		 += -D__LIBDL_SHARED__ \
		    -DUCLIBC_RUNTIME_PREFIX=\"/\"
CRT0		 =

# for debugging don't edit this file buf create ./Makeconf.local with DEBUG=1
DEBUG		 ?= 0
ifeq (,$(filter 0 n N, $(DEBUG)))
ifeq (2,$(DEBUG))
DEFINES		+= -D__SUPPORT_LD_DEBUG_EARLY__ -D__SUPPORT_LD_DEBUG__ \
		   -DDEBUG_LEVEL=2
else
DEFINES		+= -DDEBUG_LEVEL=1
endif
endif

vpath %.c $(PKGDIR)/lib/ldso/ARCH-$(ARCH)
vpath %.S $(PKGDIR)/lib/ldso/ARCH-$(ARCH)
SRC_C_libld-l4.s.so = ldso.c emul_linux.c infopage.c helper.c log_init.c \
		      binary_name.c
SRC_S_libld-l4.s.so = resolve.S
ifneq ($(filter x86 amd64,$(ARCH)),)
SRC_C_libld-l4.s.so += debug_info.c malloc.c 
endif
ifeq ($(ARCH),x86)
SRC_C_libld-l4.s.so += syscall.c
endif
LDFLAGS		 = -Bsymbolic --warn-common --export-dynamic --sort-common \
		   --discard-locals --discard-all --no-undefined -e _start \
		   --gc-sections
LIBS		 = -ldm_generic.p -ldm_mem.p -lgeneric_fprov.p \
		   -lgeneric_ts.p -llogserver.p -lnames.p $(ROOTPLIB) \
		   -ll4util.p -lsigma0.p -llist_alloc.p -ll4sys.p $(GCCLIB)
ifneq ($(filter x86 amd64,$(ARCH)),)
LIBS		+= -ldemangle.p
endif
OPTS		 = -g -Os -fno-strict-aliasing

include $(L4DIR)/mk/lib.mk

ifneq ($(SYSTEM),)
# additionally to linking TARGET to $(L4DIR)/lib (needed for linking at
# build time), copy TARGET to the binary directory (needed for linking at
# runtime)
BINDIR		?= $(DROPS_STDDIR)/bin/$(subst -,/,$(SYSTEM))
BINDIR_LOCAL	?= $(OBJ_BASE)/bin/$(subst -,/,$(SYSTEM))
INSTALLFILE_BIN_LOCAL ?= cp $(1) $(2) && chmod 644 $(2)

all:: $(BINDIR_LOCAL)/$(TARGET)
install:: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET) $(BINDIR_LOCAL)/$(TARGET): $(TARGET)
	@$(INSTALL_LOCAL_MESSAGE)
	$(if $(INSTALLFILE_BIN_LOCAL),$(VERBOSE)$(INSTALL) -d $(dir $@))
	$(VERBOSE)$(call INSTALLFILE_BIN_LOCAL,$<,$@)

.PHONY: $(BINDIR)/$(TARGET)
endif
