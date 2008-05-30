PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2

TARGET		= libnetboot.a
NOTARGETSTOINSTALL = y

SRC_C		= fsys_tftp.c nic.c netboot.c config.c misc.c gunzip.c \
		  timer.c 
SRC_C		+= ore.c 

DEFINES		= -DWITHOUT_LIBC_STUBS -DFSYS_TFTP=1
CFLAGS		= -fno-strict-aliasing
PRIVATE_INCDIR	= $(OBJ_DIR)


include $(L4DIR)/mk/lib.mk
