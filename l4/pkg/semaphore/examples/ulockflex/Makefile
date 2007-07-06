# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =		main.c

# target
TARGET =	ulockflex
MODE =		l4env
SYSTEMS =	x86-l4v2
DEFAULT_RELOC = 0x00440000

# uncomment one of the following lines to enable the respective lock
# implementation

# semaphore lib
CFLAGS += -DLOCAL_SEM
# l4util lock
#CFLAGS += -DL4UTIL_LOCK
# semaphore lib without serializer thread
#CFLAGS += -DLOCAL_DP_SEM_NOSERTHREAD

# include prog role
include $(L4DIR)/mk/prog.mk
