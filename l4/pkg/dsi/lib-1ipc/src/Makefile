# directories we need to know
PKGDIR ?=       ../..
L4DIR ?=        $(PKGDIR)/../..

# target
TARGET =	libdsi-1ipc.a
SYSTEMS =	x86-l4v2 x86-l4x0

# source files
SRC_C =		libdsi.c dataspace.c packet.c socket.c sync.c thread.c \
		app.c jcp.c event.c select.c convenience.c error.c

PRIVATE_INCDIR = ../../include

# include lib role
include $(L4DIR)/mk/lib.mk


