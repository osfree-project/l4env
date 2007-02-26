PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

TARGET		 = libld-l4.s.so
MODE		 = l4env
SYSTEMS		 = x86-l4v2 x86-l4x0 arm-l4x0
PRIVATE_INCDIR	 = $(PKGDIR)/lib/ldso \
		   $(PKGDIR)/lib/ldso/include \
		   $(PKGDIR)/lib/ldso/include/sysdeps/linux/$(LDSO_ARCH_NAME) \
		   $(PKGDIR)/lib/ldso/include/sysdeps/linux/common \
		   $(PKGDIR)/lib/ldso/include/ldso/$(LDSO_ARCH_NAME) \
		   $(PKGDIR)/lib/ldso/include/ldso
LDSO_ARCH_NAME_x86 = i386
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
OBJS_libld-l4.s.so = resolve.s.o ldso.s.o emul_linux.s.o infopage.s.o \
		     helper.s.o
LDFLAGS		 = -Bsymbolic --warn-common --export-dynamic --sort-common \
		   --discard-locals --discard-all --no-undefined -e _start
LIBS		 = -ldm_generic.p -ldm_mem.p -lgeneric_fprov.p \
		   -llogserver.p -lnames.p $(ROOTPLIB) -ll4util.p -ll4sys \
		   $(GCCLIB)
OPTS		 = -Os -fno-strict-aliasing

include $(L4DIR)/mk/lib.mk

ifneq ($(SYSTEM),)
# additionally to linking TARGET to $(L4DIR)/lib (needed for linking at
# build time), copy TARGET to the binary directory (needed for linking at
# runtime)
BINDIR		?= $(DROPS_STDDIR)/bin/$(subst -,/,$(SYSTEM))
BINDIR_LOCAL	?= $(L4DIR)/bin/$(subst -,/,$(SYSTEM))
INSTALLFILE_BIN_LOCAL ?= cp $(1) $(2) && chmod 644 $(2)

all:: $(BINDIR_LOCAL)/$(TARGET)
install:: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET) $(BINDIR_LOCAL)/$(TARGET): $(TARGET)
	@$(INSTALL_LOCAL_MESSAGE)
	$(if $(INSTALLFILE_BIN_LOCAL),$(VERBOSE)$(INSTALL) -d $(dir $@))
	$(VERBOSE)$(call INSTALLFILE_BIN_LOCAL,$<,$@)

.PHONY: $(BINDIR)/$(TARGET)
endif
