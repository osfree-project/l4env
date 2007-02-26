# directories we need to know
PKGDIR		= .
L4DIR		?= $(PKGDIR)/../..

ifneq ($(filter clean cleanall,$(MAKECMDGOALS)),)
TARGET		= lib lib-1ipc
endif

# include subdir role
include $(L4DIR)/mk/subdir.mk
