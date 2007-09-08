PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

# the name of your library
TARGET	= $(PKGNAME)_rsaref2.a
BUILD_PIC = $(TARGET)
SYSTEMS = x86

# list your .c files here
SRC_C	= $(addprefix source/,desc.c digit.c md2c.c md5c.c nn.c prime.c \
                              rsa.c r_encode.c r_dh.c r_enhanc.c r_keygen.c \
			      r_random.c r_stdlib.c)

# if your library implements the client side of an idl defined in an
# idl-file of your package, list the idl file name(s) here (no path needed)
CLIENTIDL =

PRIVATE_INCDIR	= $(PKGDIR)/include/rsaref2

WARNINGS = -Wall -Wstrict-prototypes

include $(L4DIR)/mk/lib.mk
