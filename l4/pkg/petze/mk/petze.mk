
#
# Introduce an olle Petze into the program
#

include $(L4DIR)/mk/Makeconf

LIBS    += -lpetze
LDFLAGS += -Wl,-wrap,malloc,-wrap,free,-wrap,calloc,-wrap,realloc -lpetze

ifeq (0,1)
#
# For self-defined pool names you include the following into your Makefile for
# files specified in $(SRC).

# ===> BEGIN
# XXX GCC version 2 behaves really strange
include $(L4DIR)/mk/Makeconf
ifeq ($(GCCMAJORVERSION), 2)
  DEFINES += -include $(L4DIR)/../oskit10/oskit/c/stdlib.h
  DEFINES_<srcfile> = -D'PETZE_POOLNAME="<name>"' -include $(L4DIR)/pkg/petze/include/petze.h
else
  DEFINES += -include stdlib.h
  DEFINES_<srcfile> = -D'PETZE_POOLNAME="<name>"' -include l4/petze/petze.h
endif
# <=== END

endif
