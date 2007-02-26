PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
TARGET		= libl4rm.a
MODE		= l4env
SYSTEMS		= x86-l4v2 arm-l4x0 amd64-l4v2

CLIENTIDL	= l4rm.idl
SERVERIDL	= l4rm.idl
SRC_C		= libl4rm.c config.c alloc.c avl_tree.c avl_tree_alloc.c \
		  region.c region_tree.c region_alloc.c pagefault.c \
		  attach.c detach.c reserve.c release.c setup.c \
		  lookup.c userptr.c
PRIVATE_INCDIR	= $(SRC_DIR)/../include 
#DEBUG		= 1

include $(L4DIR)/mk/lib.mk


