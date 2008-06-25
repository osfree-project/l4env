PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= lyon-tis lyon-legacy lyon-ux lyon-emu

# the default relocation address. This may be superseded by a STATIC file.
DEFAULT_RELOC	= 0x01900000

# use TrustedFS block-server stub to access untrusted storage
USE_TFS_STUB ?= 0

# some defines
DEFINES += -DUSE_TFS_STUB=$(USE_TFS_STUB)

# list your .c files here
SRC_C		= main.c init.c server.c seal_anchor.c lyon.c

# if your server implements the server side of an idl defined in an idl-file
# of your package, list the idl file name(s) here (no path needed)
SERVERIDL	= lyon.idl

# list additional library paths and libraries here
LIBS		= -ltcg -ltcg_crypt -ltcg -lcrypto_sha1_openssl -lcrypto_rsaref2 \
		  -lcrypto_pad -lcrypto_aes_linux_586.o -lcrypto_modes
LIBS_lyon-tis	= -lstpm-l4-tis.o -lio
LIBS_lyon-legacy= -lstpm-l4.o -ldde_linux26 -lddekit -lio -llist_alloc
LIBS_lyon-ux    = -lstpm-ux -lclxfux.o
LIBS_lyon-emu   = -lstpm-emu -lclxfux.o

ifneq ($(USE_TFS_STUB),0)
LIBS += -ltrustedfs-block
endif


# we need some special include dirs
PRIVATE_INCDIR = $(PKGDIR)/include

WARNINGS = -Wall

include $(L4DIR)/mk/prog.mk
