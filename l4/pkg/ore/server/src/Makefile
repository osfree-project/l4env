PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= $(PKGNAME)
DEFAULT_RELOC	= 0x01800000

SYSTEMS = x86-l4v2

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
		   local_deliver.c

DEFINES 	+= -DL4 -D__KERNEL__ -D__ORE__ -DDDE_LINUX

-include $(PKGDIR_OBJ)/server/Makeconf.bid.local
.general.d: $(PKGDIR_OBJ)/server/Makeconf.bid.local

ifeq ($(CONFIG_ORE_DDE24),)

# Most paths & co. come from DDE's Makeconf file
include $(PKGDIR)/../dde/linux26/Makeconf

PRIVATE_INCDIR += $(PKGDIR_ABS)/server/include \
		 		  $(PKGDIR_OBJ)/server/include \

LIBS 		+= -llinuxnetdrv26.o -ldde_linux26_net -ldde_linux26.o -lddekit

else # using DDE2.4

PRIVATE_INCDIR += $(PKGDIR_ABS)/server/include/linux_headers 	\
		 $(DDE_INCDIR)					\
		 $(LINUX24_INCDIR)				\
		 $(PKGDIR_ABS)/server/include			\
		 $(PKGDIR_OBJ)/server/include
PRIVATE_LIBDIR = $(PKGDIR_OBJ)/server/lib/linuxemul/OBJ-$(SYSTEM)
LIBS 		+= -llinuxemul -llinuxnetdrv -llinuxemul -ldde_linux
LDFLAGS		+= --whole-archive \
		   $(PKGDIR_OBJ)/server/lib/net/OBJ-$(SYSTEM)/liblinuxnetdrv.a \
		   --no-whole-archive

endif

ifeq ($(CONFIG_ORE_FERRET),y)
LIBS        += -lferret_client -lferret_producer -lferret_client -lferret_util \
			   -lferret_comm -lferret_fpages -lferret_local_names
endif

LIBCINCDIR     += -nostdinc $(I_GCCINCDIR)

LIBS		+= -levents -lomega0 -lio -ll4rm -lore_server-worker \
		   -lore_server -lore_notify -llist_alloc -lparsecmdline

ifeq ($(CONFIG_ORE_UX_ONLY),y)
LIBS += -lclxfux.o $(GCCLIB)
else
LIBS += $(GCCLIB)
endif

ifeq ($(CONFIG_ORE_PACKET_TRACE),y)
LIBS += -lpacket_debug
endif

ifeq ($(USE_DSI),y)
SRC_C	  += dsi.c rxtx_dsi.c
CFLAGS	  += -DORE_DSI
LIBS	  += -ldsi
else
SRC_C     += dummy_dsi.c
endif

include $(L4DIR)/mk/prog.mk
