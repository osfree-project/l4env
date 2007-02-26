L4DIR ?= ../..

include $(L4DIR)/mk/Makeconf

all:: lib examples

doc:

include:

idl:

lib:
	make -C ../uclibc/lib/uclibc SYSTEMS="x86-l4v2 arm-l4x0" all

examples:
	$(VERBOSE)make -C examples

install clean cleanall:
	make -C ../uclibc/lib/uclibc SYSTEMS="x86-l4v2 arm-l4x0" $@
	make -C examples $@

.PHONY: examples
