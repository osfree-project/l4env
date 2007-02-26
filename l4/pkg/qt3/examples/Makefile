PKGDIR ?= ..
L4DIR  ?= $(PKGDIR)/../..

# create examples demonstrating the use of you package in subdirectories
# and list those subdirs in the TARGET variable.
TARGET = data application widgets qtop qtinyeditor qws_client_server

# 'uic' is not installed on all systems (e.g. on os.inf.tu-dresden.de).
# To build also examples requiring this tool, add 'HAVE_UIC = 1' to your
# Makeconf.local.
-include Makeconf.local

ifeq ($(HAVE_UIC),1)
    TARGET += ftpclient qrun qvv sheep
endif


include $(L4DIR)/mk/subdir.mk


