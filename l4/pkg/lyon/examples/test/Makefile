PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= lyon_test

# the default relocation address. This may be superseded by a STATIC file.
DEFAULT_RELOC	= 0x01200000

# list your .c files here
SRC_C		= test.c

# if your server implements the server side of an idl defined in an idl-file
# of your package, list the idl file name(s) here (no path needed)
SERVERIDL	=

# list additional library paths and libraries here 
LIBS		= -llyon-client

# we need some special dirs
PRIVATE_INCDIR = $(PKGDIR)/include

WARNINGS = -Wall


include $(L4DIR)/mk/prog.mk
