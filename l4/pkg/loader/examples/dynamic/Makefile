PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= test_dyn
SUBDIRS		= libfoo_dyn
MODE		= loader
LIBS		= -lparsecmdline
DEFAULT_RELOC	= 0x01000000
SRC_C		= main.c
SYSTEMS		= x86-l4v2 x86-l4x0

# The -E flag is essential here to make sure that _all_ symbols of
# the binary (test_dyn) are exeported as dynamic symbols. We need
# this option since we load a dynamic library after startup
LDFLAGS		= -Wl,-E

all:: $(SUBDIRS)

include $(L4DIR)/mk/prog.mk
