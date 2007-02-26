PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

MODE		= sigma0
TARGET		= list_alloc_test list_alloc_stress
SRC_C_list_alloc_test   = test.c
SRC_C_list_alloc_stress = stress.c
DEFAULT_RELOC	= 0x00800000
SYSTEMS		= x86-l4v2 x86-l4x0
LIBS		= -llist_alloc

include $(L4DIR)/mk/prog.mk
