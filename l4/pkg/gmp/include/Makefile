PKGDIR          ?= ..
L4DIR           ?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

SYSTEMS         = x86-l4v2
INCSRC_DIR      = $(firstword $(wildcard $(PKGDIR_OBJ)/lib/build/OBJ-*))
TARGET          = $(if $(wildcard $(INCSRC_DIR)/gmp.h),gmp.h,)

include $(L4DIR)/mk/include.mk
