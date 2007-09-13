PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= $(PKGNAME)-l4 $(PKGNAME)-l4-tis $(PKGNAME)-ux $(PKGNAME)-emu

# the default relocation address. This may be superseded by a STATIC file.
DEFAULT_RELOC	= 0x01700000

# list your .c files here
SRC_C 	 	= server.c

# if your server implements the server side of an idl defined in an idl-file
# of your package, list the idl file name(s) here (no path needed)
SERVERIDL	= stpm.idl

# list additional library paths and libraries here
LIBS_stpm-l4		= -lstpm-l4.o -ldde_linux26 -lddekit -lio -llist_alloc
LIBS_stpm-l4-tis	= -lstpm-l4-tis.o -lio
LIBS_stpm-ux 		= -lstpm-ux -lclxfux.o -llist_alloc
LIBS_stpm-emu 		= -lstpm-emu -lclxfux.o -llist_alloc

# we need some special dirs
PRIVATE_INCDIR = $(PKGDIR)/include

WARNINGS = -Wall -Wstrict-prototypes


include $(L4DIR)/mk/prog.mk
