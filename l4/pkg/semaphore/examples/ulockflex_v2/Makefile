# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C		= main.c

# target
TARGET	 	= ulockflex_v2
MODE		= l4env
SYSTEMS		= x86-l4v2 arm-l4v2 amd64-l4v2
DEFAULT_RELOC	= 0x00480000

# uncomment one of the following lines to enable the respective lock
# implementation

#semaphore with kernel support
CFLAGS += -DFIASCO_USEM
#CFLAGS += -DFIASCO_ULOCK

# semaphore lib with serializer/tamer thread
#CFLAGS += -DLOCAL_SEM

# l4util lock
#CFLAGS += -DL4UTIL_LOCK

# semaphore without serializer thread based on delayed preemption
#CFLAGS += -DLOCAL_DP_SEM_NOSERTHREAD

# include prog role
include $(L4DIR)/mk/prog.mk
