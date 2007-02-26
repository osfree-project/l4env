PKGDIR        ?= ../..
L4DIR         ?= $(PKGDIR)/../..

TARGET         = libl4lx22dopecon.a
MODE           = l4linux_kern
WARNINGS       = -Wno-missing-prototypes -Wno-missing-declarations

SRC_C          = main.c comh.c con_kbd.c dope_kbd.c xf86if.c bootlog.c
                 #ovl_kbd.c
PRIVATE_INCDIR = $(PKGDIR)/idl/OBJ-$(SYSTEM) .

#LIBCINCDIR      = -nostdinc -I$(GCCINCDIR) -I$(LINUX24_INCDIR) -D__KERNEL__ -DLINUX_ON_L4
LIBCINCDIR      = -nostdinc -I$(GCCINCDIR) -I$(L4LINUX22_DIR)/include \
		  -D__KERNEL__ -DLINUX_ON_L4

CLIENTIDL      = dope.idl pslim.idl
SERVERIDL      = dopeapp.idl #input_listener.idl


vpath dope-client.c    $(L4DIR)/pkg/dope/idl/OBJ-$(SYSTEM)
vpath pslim-client.c   $(L4DIR)/pkg/dope/idl/OBJ-$(SYSTEM)
vpath dopeapp-server.c $(L4DIR)/pkg/dope/idl/OBJ-$(SYSTEM)

include $(L4DIR)/mk/lib.mk
