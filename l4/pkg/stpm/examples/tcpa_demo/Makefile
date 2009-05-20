PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= tcpa_demo tcpa_demo-ux tcpa_demo-emu tcpa_demo-l4 tcpa_demo-l4-tis

# the default relocation address. This may be superseded by a STATIC file.
DEFAULT_RELOC	= 0x01000000

# list your .c files here
SRC_C		= tcpa_demo.c

# if your server implements the server side of an idl defined in an idl-file
# of your package, list the idl file name(s) here (no path needed)
SERVERIDL	=

# list additional library paths and libraries here
LIBS			= -ltcg -ltcg_crypt -ltcg -lcrypto_sha1_linux
LIBS_tcpa_demo		= -lstpm-client -llist_alloc
LIBS_tcpa_demo-ux	= -lstpm-ux -lclxfux.o
LIBS_tcpa_demo-emu	= -lstpm-emu -lclxfux.o
LIBS_tcpa_demo-l4	= -lstpm-l4.o -ldde_linux26.o -lddekit -lio -llist_alloc
LIBS_tcpa_demo-l4-tis	= -lstpm-l4-tis.o -lio


# we need some special dirs
PRIVATE_INCDIR = $(PKGDIR)/include

WARNINGS = -Wall


include $(L4DIR)/mk/prog.mk
