PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf
-include $(PKGDIR_OBJ)/Makeconf

ifeq ($(CONFIG_DDE26),y)
TARGET  = src
endif

include $(L4DIR)/mk/subdir.mk
