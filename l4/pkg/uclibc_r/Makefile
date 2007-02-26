all: lib examples

doc:

idl lib:
	make -C ../uclibc/lib/uclibc SYSTEMS="x86-l4v2 x86-l4x0 arm-l4x0" all

examples:
	$(VERBOSE)make -C examples $@

install clean cleanall:
	make -C ../uclibc/lib/uclibc SYSTEMS="x86-l4v2 x86-l4x0 arm-l4x0" $@
	make -C examples $@
