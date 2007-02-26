PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= $(PKGNAME)
# the default relocation address. This may be superseded by a STATIC file.
DEFAULT_RELOC	= 0x01800000

# list your .c files here
SRC_C		= main.c

# if your server implements the server side of an idl defined in an idl-file
# of your package, list the idl file name(s) here (no path needed)
SERVERIDL	=

# list additional library paths and libraries here
LIBS		=

include $(L4DIR)/mk/prog.mk
