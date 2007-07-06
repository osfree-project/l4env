PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = ovltrack
MODE             = l4linux
SYSTEMS          = x86-l4v2

LIBCINCDIR       = $(addprefix -I,$(L4INCDIR))
LIBS             = -ll4lxovlwindow -lsemaphore -lsigma0 \
                   -lXext -lX11 -pthread

PRIVATE_INCDIR   = $(PKGDIR)/include \
                   $(XF86TREE)/include \
                   $(XF86TREE)/lib \
                   $(PKGDIR)/examples/xf86track/include

PRIVATE_LIBDIR   = $(XF86TREE)/lib /usr/X11R6/lib /usr/lib/gcc-lib/i486-linux/3.3.5

SRC_C            = main.c logfake.c

vpath logfake.c $(PKGDIR)/examples/xf86wm/l4lx

include $(L4DIR)/mk/prog.mk
