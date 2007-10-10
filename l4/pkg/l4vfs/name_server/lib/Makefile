PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..

SYSTEMS  = x86-l4v2 amd64-l4v2 arm-l4v2

TARGET   = libl4vfs_name_server.a

SRC_C_libl4vfs_name_server.a = name_server.c
SRC_C_libl4vfs_name_server.p.a = $(SRC_C_libl4vfs_name_server.a)

MODE     = l4env_minimal

include $(L4DIR)/mk/lib.mk
