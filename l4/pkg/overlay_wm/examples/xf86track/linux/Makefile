PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = ovltrack
MODE             = l4linux
SYSTEMS          = x86-l4v2 x86-l4x0

LIBCINCDIR       = $(addprefix -I,$(L4INCDIR))
LIBS             = -ll4ovlwindow -lthread_linux -lsemaphore -lthread_linux \
                   -lX11 -lXext -pthread

XROOT            = /usr/X11R6
PRIVATE_INCDIR   = $(PKGDIR)/include \
                   $(XROOT)/include \
                   $(PKGDIR)/examples/xf86track/include
PRIVATE_LIBDIR   = $(XROOT)/lib

SRC_C            = main.c logfake.c ovldump.c

vpath logfake.c $(PKGDIR)/examples/xf86wm/l4lx
vpath main.c    $(PKGDIR)/examples/xf86track/l4lx

include $(L4DIR)/mk/prog.mk
