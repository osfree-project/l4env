L4DIR ?= ../..

include $(L4DIR)/mk/Makeconf

all:: lib examples

doc:

include:

idl:

lib:
	$(VERBOSE)PWD=$(call absfilename,../uclibc/lib/uclibc) \
	   $(MAKE) -C ../uclibc/lib/uclibc SYSTEMS="x86-l4v2 arm-l4x0" all

examples:
	$(VERBOSE)PWD=$(PWD)/examples $(MAKE) -C examples

install clean cleanall:
	$(VERBOSE)PWD=$(call absfilename,../uclibc/lib/uclibc) \
	  $(MAKE) -C ../uclibc/lib/uclibc SYSTEMS="x86-l4v2 arm-l4x0 amd64-l4v2" $@
	$(VERBOSE)PWD=$(PWD)/examples $(MAKE) -C examples $@

.PHONY: examples
