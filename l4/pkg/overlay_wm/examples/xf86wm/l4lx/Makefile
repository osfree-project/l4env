PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = ovlwm
MODE             = l4linux
SYSTEMS          = x86-l4v2 x86-l4x0

LIBCINCDIR       = $(addprefix -I,$(L4INCDIR))
#LIBS             = -ll4lxovlwindow -lnames -ll4util -lrmgr -ldm_mem -lX11 -lXext -pthread 
LIBS             = -ll4ovlwindow -lthread_linux -lsemaphore -lthread_linux \
                   -lX11 -lXext -pthread

XROOT            = /usr/X11R6
PRIVATE_INCDIR   = $(PKGDIR)/include \
                   $(XROOT)/include \
                   $(PKGDIR)/examples/xf86wm/include
PRIVATE_LIBDIR   = $(XROOT)/lib

SRC_C            = init.c events.c client.c new.c manage.c xmisc.c logfake.c

include $(L4DIR)/mk/prog.mk
