PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libloader.s.so
MODE		= l4env
SYSTEMS		= x86-l4v2 x86-l4x0
vpath %.c $(L4DIR)/pkg/l4env/lib/src


# When -Bsymbolic is added to the linker flags, all references to functions
# and data are resolved using the stuff provided by libloader. This would
# make it impossible 
# - to overwrite weak symbols defined in libloader
# - to access data defined in libloader!
#
# LDFLAGS	= -Bsymbolic

OBJS_libloader.s.so = main.s.o helper_dyn.s.o errstr-l4ipc.s.o
LIBS		= --whole-archive \
		    -lsemaphore.p \
		    -lthread.p \
		    -ldm_generic.p \
		    -ldm_mem.p \
		    -lslab.p \
		    -ll4env_err.p \
		    -ll4rm.p \
		    -ll4env_loaderruntime.p \
		    -llogserver.p \
		    -lnames.p \
		    -lrmgr.p \
		    -lgeneric_ts.p \
		    -lloaderif.p \
		  --no-whole-archive \
		    -ll4util.p \
		    $(GCCLIB)

include $(L4DIR)/mk/lib.mk

ifneq ($(SYSTEM),)
BINTARGET	?= $(DROPS_STDDIR)/bin/$(subst -,/,$(SYSTEM)/$(TARGET))
BINTARGET_LOCAL	?= $(L4DIR)/bin/$(subst -,/,$(SYSTEM)/$(TARGET))
INSTALLFILE_BIN_LOCAL ?= cp $(1) $(2) && chmod 644 $(2)

all:: $(BINTARGET_LOCAL)
install:: $(BINTARGET)

$(BINTARGET) $(BINTARGET_LOCAL): $(TARGET)
	@$(INSTALL_LOCAL_MESSAGE)
	$(if $(INSTALLFILE_BIN_LOCAL),$(VERBOSE)$(INSTALL) -d $(dir $@))
	$(VERBOSE)$(call INSTALLFILE_BIN_LOCAL,$<,$@)

.PHONY: $(BINTARGET)
endif
