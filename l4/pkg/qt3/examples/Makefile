PKGDIR ?= ..
L4DIR  ?= $(PKGDIR)/../..

# create examples demonstrating the use of you package in subdirectories
# and list those subdirs in the TARGET variable.
TARGET = qws_client_server

# 'moc' and 'uic' are not installed on all systems (e.g. on os.inf.tu-dresden.de).
# To build also examples requiring these tools, add 'HAVE_MOC_UIC = 1' to your
# Makeconf.local.
-include Makeconf.local

ifeq ($(HAVE_MOC_UIC),1)
    TARGET += application ftpclient qrun qtop qvv 
endif


include $(L4DIR)/mk/subdir.mk


