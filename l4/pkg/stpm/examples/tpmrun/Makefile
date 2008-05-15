PKGDIR        ?= ../..
L4DIR         ?= $(PKGDIR)/../..

DEPENDS_PKGS   = l4con

TARGET         = tpmrun

# the default relocation address. This may be superseded by a STATIC file.
DEFAULT_RELOC	 = 0x01000000

# list your .c files here
SRC_C          = main.c createkey.c quote.c dumpkey.c fprov.c

# list additional library paths and libraries here 
LIBS           = -ltcg -ltcg_crypt -lcrypto_sha1_linux -lstpm-client \
                 -lcrypto_pad -lcrypto_rsaref2 \
                 -lcontxt -lconstream-server -lgeneric_fprov

# we need some special dirs
PRIVATE_INCDIR = $(PKGDIR)/include \
                 $(PKGDIR)/contrib/include \
                 $(PKGDIR)/contrib/libtcpa

WARNINGS       = -Wall


include $(L4DIR)/mk/prog.mk
