# directories we need to know
PKGDIR ?=	../../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =		libblk.c driver.c request.c notification.c ctrl.c
PRIVATE_INCDIR =$(SRC_DIR)/../include
CLIENTIDL = 	blk.idl

# target
TARGET = 	libgeneric_blk.a
SYSTEMS =	x86-l4v2

# include lib role
include $(L4DIR)/mk/lib.mk
