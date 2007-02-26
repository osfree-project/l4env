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
		   util.c

DEFINES 	= -DL4 -D__KERNEL__ -D__ORE__ -DDDE_LINUX	   	

PRIVATE_INCDIR = $(PKGDIR)/server/include/linux_headers	 	\
		 $(DDE_INCDIR)					\
		 $(LINUX24_INCDIR)				\
		 $(PKGDIR)/server/include
LIBCINCDIR     = -nostdinc -I$(GCCINCDIR)

PRIVATE_LIBDIR = $(PKGDIR)/server/lib/linuxemul/OBJ-$(SYSTEM)

LIBS		+= -llinuxemul -levents -llinuxnetdrv -ldde_linux \
		   -lomega0 -lio -ll4rm -lore_server-internal \
		   -lore_server -lore_notify -llist_alloc -lparsecmdline \
		   $(GCCLIB)

LDFLAGS		+= -Wl,--whole-archive \
		   $(PKGDIR)/server/lib/net/OBJ-$(SYSTEM)/liblinuxnetdrv.a \
		   -Wl,--no-whole-archive

include $(L4DIR)/mk/prog.mk
