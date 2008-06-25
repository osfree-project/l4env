PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= quotes

# the default relocation address. This may be superseded by a STATIC file.
DEFAULT_RELOC	= 0x01000000

# list your .c files here
SRC_C		= quote_demo.c

# if your server implements the server side of an idl defined in an idl-file
# of your package, list the idl file name(s) here (no path needed)
SERVERIDL	=

# list additional library paths and libraries here 
LIBS_quotes		= -ltcg -ltcg_crypt -ltcg -lcrypto_sha1_linux -lstpm-client

# we need some special dirs
PRIVATE_INCDIR = $(PKGDIR)/include \
                 $(PKGDIR)/contrib/include \
                 $(PKGDIR)/contrib/libtcpa

WARNINGS = -Wall


include $(L4DIR)/mk/prog.mk
