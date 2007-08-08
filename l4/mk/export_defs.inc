# Makefile snippet to generate l4defs.inc file
# vim:set ft=make:
#

PKGDIR = .
SYSTEMS = $(BUILD_ARCH)-$(BUILD_ABI)
MODE	= l4env

OBJ_DIR = $(SRC_DIR)/build

ifneq ($(SYSTEM),)

# take crt0.o from the install directory
L4_CRT0     = $(patsubst $(L4_BUILDDIR)%, $(DROPS_STDDIR)/%, $(CRT0))
# use the DICE directory in the install directory
L4_DICEDIR  = $(patsubst $(L4DIR)/%, $(DROPS_STDDIR)/%, $(DICE_INCDIR))
# let LDFLAGS and CFLAGS only contain directories within the install path
L4_LDFLAGS  = $(filter-out -L$(L4_BUILDDIR)%, $(patsubst -T$(L4_BUILDDIR)%, -T$(DROPS_STDDIR)/%, $(LDFLAGS)))
L4_CPPFLAGS = $(filter-out -I$(L4_BUILDDIR)%, $(patsubst -I$(DICE_INCDIR), -I$(L4_DICEDIR), $(CPPFLAGS)))

# the target file
L4DEF_FILE ?= l4defs.inc

all::
	@echo -e $(EMPHSTART)"Creating $(L4DEF_FILE)"$(EMPHSTOP)
	@echo "# Definitions for L4 BID"                         > $(L4DEF_FILE)
	@echo "# --- Automatically created on install, do not modify ---" >> $(L4DEF_FILE)
	@echo "L4_CRT0 = $(L4_CRT0)"                            >> $(L4DEF_FILE)
	@echo "L4_LDFLAGS = $(L4_LDFLAGS)"                      >> $(L4DEF_FILE)
	@echo "L4_CPPFLAGS = $(L4_CPPFLAGS)"                    >> $(L4DEF_FILE)
else
all::
endif

include $(L4DIR)/mk/prog.mk
