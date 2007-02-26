PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

#PKGNAME		= dietlibc
TARGET		= $(shell find . -name \*.h -printf "%P ") \
		  cassert cstdio cstdlib cctype cstddef csetjmp climits csetjmp \
		  cstring cstdarg

INSTALL_INC_PREFIX=$(PKGNAME)

include $(L4DIR)/mk/include.mk
