PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..
SYSTEMS ?= x86-l4v2 x86-l4x0
DOPEDIR ?= $(L4DIR)/pkg/dopeng
MODE     = host
TARGET   = fnt2tff
SRC_C    = main.c pool.c conv_fnt.c dopestd.c
PRIVATE_INCDIR = $(DOPEDIR)/server/include

vpath pool.c     $(DOPEDIR)/server/common
vpath conv_fnt.c $(DOPEDIR)/server/common
vpath dopestd.c  $(DOPEDIR)/server/common

include $(L4DIR)/mk/prog.mk
