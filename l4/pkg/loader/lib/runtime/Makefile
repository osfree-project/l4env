PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libloader.s.so
MODE		= l4env
SYSTEMS		= x86-l4v2 x86-l4x0 arm-l4x0

# When -Bsymbolic is added to the linker flags, all references to functions
# and data are resolved using the stuff provided by libloader. This would
# make it impossible 
# - to overwrite weak symbols defined in libloader
# - to access data defined in libloader!
#
# LDFLAGS	= -Bsymbolic

include $(L4DIR)/mk/Makeconf

LOADERLIBS_oskit=
LOADERLIBS_diet	= -lc_be_l4env_start_stop.p	\
		  -lc_be_minimal_log_io.p	\
		  -lc_be_simple_mem.p		\
		  -lc_be_mmap.p			\
		  -lc_be_mmap_util.p
LOADERLIBS_uc	= -lc_be_l4env_start_stop.p	\
		  -lc_be_minimal_log_io.p	\
		  -lc_be_simple_mem.p		\
		  -lc_be_mmap.p			\
		  -lc_be_mmap_util.p		\
		  -luclibc_support.p

vpath %.c $(L4DIR)/pkg/l4env/lib/src
CRT0		=
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
		    $(ROOTPLIB) \
		    $(LOADERLIBS_$(LIBC_TYPE)) \
		    -lgeneric_ts.p \
		    -lloaderif.p \
		  --no-whole-archive \
		    -ll4util.p -ul4util_reboot -ugetopt_long \
		    $(GCCLIB)

# NOTE on libl4util: libl4util has dependencies to malloc which isn't
#                    always desirable. So we don't put it into the
#                    whole-archive group but outside and explicitly include
#                    symbols used by applications which are not referenced
#                    by the other libraries included in libloader.

LDFLAGS_arm     = -Wl,--no-warn-mismatch
LDFLAGS         = $(LDFLAGS_$(ARCH))

include $(L4DIR)/mk/lib.mk

ifneq ($(SYSTEM),)
# additionally to linking TARGET to $(L4DIR)/lib (needed for linking at
# build time), copy TARGET to the binary directory (needed for linking at
# runtime)
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
