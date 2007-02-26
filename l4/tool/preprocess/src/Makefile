#
# Makefile for the sources of the smart_mpeg library
#

# scan the L4 global configuration file
ifeq (${L4_DIR},)
  L4DIR = ../../..
else
  L4DIR = ${L4_DIR}
endif
include $(L4DIR)/Makeconf

-include Makeconf.local

TARGET = preprocess

all: $(TARGET)

install: all
	-$(INSTALL) -d $(L4_INSTALL_DIR)/tool/bin
	$(INSTALL) -m 755 $(TARGET) $(L4_INSTALL_DIR)/tool/bin

clean:: 
#	$(RM) $(TARGET) $(OBJS) tmp.*

cleanall::      clean
	$(RM) *~ .*.d

#include $(DEPS)
