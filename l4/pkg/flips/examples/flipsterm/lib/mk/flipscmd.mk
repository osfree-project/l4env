#
# TEMPLATE FOR PROGRESSIVE BUILDING OF FLIPSCMDS
#

ifeq ($(FLIPSCMD),)
$(error Variable FLIPSCMD is not defined)
endif

TARGET            = lib$(FLIPSCMD).a
TARGET_O          = $(TARGET:.a=.o)
PRIVATE_INCDIR   += $(PKGDIR)/examples/flipsterm/lib/include
INSTALL_TARGET    =
SRC_C            += printf.c

vpath printf.c $(PKGDIR)/examples/flipsterm/lib/common

include $(L4DIR)/mk/lib.mk

ifneq ($(SYSTEM),)
all:: $(TARGET_O)

$(TARGET_O):%.o:%.a
	ld --whole-archive -r $< -o $@
	objcopy --redefine-sym main=flipscmd_$(FLIPSCMD) \
	        --redefine-sym exit=flipscmd_exit \
	        -G flipscmd_$(FLIPSCMD) $@ $@
endif
