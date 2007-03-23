L4DIR ?= ../..
include $(L4DIR)/mk/Makeconf

idl include doc:

cleanall clean lib install all::
	$(VERBOSE)PWD=$(call absfilename,../dietlibc/lib/dietlibc) \
	  $(MAKE) -C ../dietlibc/lib/dietlibc/ SYSTEMS="x86-l4v2 arm-l4x0 amd64-l4v2" $@
