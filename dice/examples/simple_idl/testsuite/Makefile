ifeq (${L4_DIR},)
  L4DIR = ../../../../..
else
  L4DIR = ${L4_DIR}
endif

SRC_C = ${wildcard *.c}

TARGET_STAT = simple_test
OSKIT10 = 1
DEFAULT_RELOC = 0x00400000

DICE = $(L4DIR)/tool/dice/src/dice
DICE_MESSAGE = @echo "Compiling IDL..."
DICE_OPT = -T 
IDL_FILE = ../idl/test.idl
VERBSOE=@

PRIVATE_INCDIR += ../../../../../include ../../../include/

default all: idl $(TARGET_STAT)

include ${L4DIR}/Makeconf
include ${L4DIR}/Makeconf.oskit10_l4env_tiny

idl: $(IDL_FILE)
	$(DICE_MESSAGE)
	$(VERBOSE)$(DICE) $(L4ALL_INCDIR) $(DICE_OPT) $(IDL_FILE)

$(TARGET_STAT): $(OBJS) $(LIBDEPS) $(LDDEPS)
	$(LINK_MESSAGE)
	$(VEBROSE)$(CC) $(GCC_LDFLAGS) -o $@ $(OBJS) $(L4ALL_LIBDIR) $(L4_LIB)
	$(VERBOSE)$(STRIP) -o $@.stripped $@
ifneq ($(wildcard $(INSTDIR)),)
	$(VERBOSE)$(INSTALL) -m 755 $@.stripped $(INSTDIR)/$@
endif

clean:
	$(VERBOSE)$(RM) *.o *.c *.h

cleanall: clean
	$(VERBOSE)$(RM) $(TARGET_STAT) $(TARGET_STAT).stripped

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),cleanall)
-include $(DEPS)
endif
endif

$(SRC_C): $(IDL_FILE)
 
