PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
TARGET		 = libdl.s.so
MODE		 = l4env
SYSTEMS		 = x86-l4v2 arm-l4x0
PRIVATE_INCDIR	 = $(PKGDIR)/lib/libdl \
		   $(PKGDIR)/lib/ldso \
		   $(PKGDIR)/lib/ldso/include \
		   $(PKGDIR)/lib/ldso/include/sysdeps/linux/$(LDSO_ARCH_NAME) \
		   $(PKGDIR)/lib/ldso/include/sysdeps/linux/common \
		   $(PKGDIR)/lib/ldso/include/ldso/$(LDSO_ARCH_NAME) \
		   $(PKGDIR)/lib/ldso/include/ldso
LDSO_ARCH_NAME_x86 = i386
LDSO_ARCH_NAME_arm = arm
LDSO_ARCH_NAME	= $(LDSO_ARCH_NAME_$(ARCH))
DEFINES		 = -D__LIBDL_SHARED__ -DUCLIBC_RUNTIME_PREFIX=\"/\"
SRC_C		 = libdl.c
LDFLAGS		 = -Bsymbolic --warn-common --export-dynamic --sort-common \
		   --discard-locals --discard-all -soname=$(TARGET)
LIBS		 = -lld-l4.s
OPTS		 = -Os -fno-strict-aliasing
CRT0		 =

# for debugging don't edit this file buf create ./Makeconf.local with DEBUG=1
# be aware that enabling DEBUG here depends on DEBUG in ldso too because of
# mmap_list_regions()
DEBUG		 ?= 0

include $(L4DIR)/mk/lib.mk

ifneq ($(SYSTEM),)
BINTARGET	?= $(DROPS_STDDIR)/bin/$(subst -,/,$(SYSTEM)/$(TARGET))
BINTARGET_LOCAL	?= $(OBJ_BASE)/bin/$(subst -,/,$(SYSTEM)/$(TARGET))
INSTALLFILE_BIN_LOCAL ?= cp $(1) $(2) && chmod 644 $(2)

all:: $(BINTARGET_LOCAL)
install:: $(BINTARGET)

$(BINTARGET) $(BINTARGET_LOCAL): $(TARGET)
	@$(INSTALL_LOCAL_MESSAGE)
	$(if $(INSTALLFILE_BIN_LOCAL),$(VERBOSE)$(INSTALL) -d $(dir $@))
	$(VERBOSE)$(call INSTALLFILE_BIN_LOCAL,$<,$@)

.PHONY: $(BINTARGET)
endif
