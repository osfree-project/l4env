PKGDIR	= ..
L4DIR	?= $(PKGDIR)/../..
TARGET	= io base thread be start util tl

include $(L4DIR)/mk/subdir.mk

base: io
be: io
start: base thread util
thread: io
util: io

