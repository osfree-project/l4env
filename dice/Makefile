# This is a phony makefile.
#
# It is only used if 
# a) called directly (make -f Makefile) or
# b) no GNUmakefile exists
#
# In both cases we tell the user to generate a
# GNUmakefile using ./configure and call make
# again.
# 
ifneq ($(wildcard ./GNUmakefile),)
include ./GNUmakefile
else

all::
	$(error "Please run ./bootstrap and ./configure and then call make again.")

drops:
	$(MAKE) -f Makefile.drops

debian-pkg:
	dpkg-buildpackage -rfakeroot -uc -us -b
	
endif

