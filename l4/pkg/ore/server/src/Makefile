PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= $(PKGNAME)
DEFAULT_RELOC	= 0x01800000

# list your .c files here
SRC_C		=  main.c		\
		   openclose.c 		\
		   clients.c		\
		   recv.c 			\
		   send.c 			\
		   netdevice.c 		\
		   irq_handling.c	\
		   rxtx_string.c	\
		   debug.c			\
		   util.c			\
		   events.c			\
		   checksum.c		\
		   xmit_lock.c		\
		   worker.c			\
		   local_deliver.c	\

DEFINES 	= -DL4 -D__KERNEL__ -D__ORE__ -DDDE_LINUX	   	

PRIVATE_INCDIR = $(PKGDIR_ABS)/server/include/linux_headers 	\
		 $(DDE_INCDIR)					\
		 $(LINUX24_INCDIR)				\
		 $(PKGDIR_ABS)/server/include			\
		 $(PKGDIR_OBJ)/server/include
LIBCINCDIR     = -nostdinc -I$(GCCINCDIR)

PRIVATE_LIBDIR = $(PKGDIR_OBJ)/server/lib/linuxemul/OBJ-$(SYSTEM)

LIBS		+= -llinuxemul -levents -llinuxnetdrv -ldde_linux \
		   -lomega0 -lio -ll4rm -lore_server-worker \
		   -lore_server -lore_notify -llist_alloc -lparsecmdline

-include $(PKGDIR_OBJ)/server/Makeconf.bid.local
.general.d: $(PKGDIR_OBJ)/server/Makeconf.bid.local

ifeq ($(CONFIG_ORE_UX_ONLY),y)
LIBS += -lclxfux.o $(GCCLIB)
else
LIBS += $(GCCLIB)
endif

LDFLAGS		+= -Wl,--whole-archive \
		   $(PKGDIR_OBJ)/server/lib/net/OBJ-$(SYSTEM)/liblinuxnetdrv.a \
		   -Wl,--no-whole-archive

ifeq ($(USE_DSI),y)
SRC_C	  += dsi.c rxtx_dsi.c
CFLAGS	  += -DORE_DSI
LIBS	  += -ldsi
else
SRC_C     += dummy_dsi.c
endif

include $(L4DIR)/mk/prog.mk
