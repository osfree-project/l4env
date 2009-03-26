PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf
-include $(PKGDIR_OBJ)/Makeconf

ifeq ($(CONFIG_DDE26),y)
TARGET = dde26_test ne2k bug_timersleep
endif

include $(L4DIR)/mk/subdir.mk


