PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

# the name of your library
TARGET	= $(PKGNAME)_aes_linux_586.o.a
BUILD_PIC = $(TARGET)
SYSTEMS = x86

# list your .c files here
SRC_C	= aes.c
SRC_S	= aes-i586-asm.S

# if your library implements the client side of an idl defined in an
# idl-file of your package, list the idl file name(s) here (no path needed)
CLIENTIDL =

include $(L4DIR)/mk/lib.mk
