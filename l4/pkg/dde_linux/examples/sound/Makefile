PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2

### act the role of ...
include $(L4DIR)/mk/prog.mk

ifeq ($(DDE_LINUX_DIR),)
  $(error Please set setup path to Linux 2.4.20 sources (DDE_LINUX_DIR) in $(L4DIR)/Makeconf.local)
endif
