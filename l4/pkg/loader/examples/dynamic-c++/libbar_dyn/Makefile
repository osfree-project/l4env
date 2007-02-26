PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libbar_dyn_c++.s.so
MODE		= l4env
SYSTEMS		= x86-l4v2 arm-l4x0
OBJS		= bar.s.o
CXXFLAGS	= -fno-rtti -fno-exceptions

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
