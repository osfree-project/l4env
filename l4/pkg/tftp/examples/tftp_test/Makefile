PKGDIR               ?= ../..
L4DIR                ?= $(PKGDIR)/../..

TARGET                = tftp_test tftp_test_write
DEFAULT_RELOC         = 0x00A00000
SYSTEMS               = x86-l4v2

SRC_C_tftp_test       = main.c
SRC_C_tftp_test_write = write.c
LIBS                  = -lfprov_ext

include $(L4DIR)/mk/prog.mk
