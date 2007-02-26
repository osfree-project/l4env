PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

#PKGNAME		= uclibc
TARGET		= $(shell find . -name \*.h -printf "%P ") \
		  cassert cstdio cstdlib cctype cstddef csetjmp \
		  csetjmp cstring

INSTALL_INC_PREFIX=$(PKGNAME)

include $(L4DIR)/mk/include.mk
