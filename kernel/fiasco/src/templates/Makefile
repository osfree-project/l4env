# This makefile is a template for a build in a directory other 
# than src. 
# The only thing you have to do is to copy this template to
# your build dir (cp Makefile.builddir.templ <builddir>/Makefile)
# and change srcdir to the relative path to your fiasco/src.
#

SAMPLE_CONFIGS := ia32-short ia32-noshort ia32-ndebug \
		  ux-short ux-noshort ia32-smas\
		  arm-x0

srcdir = ../

all:

testconfig:
	$(foreach cfg,$(SAMPLE_CONFIGS), \
	  cp globalconfig.out.$(cfg) globalconfig.out ; \
          CONFIG_BANNER_STRING="\"Fiasco - $(cfg)\"" make menuconfig && \
	  mv globalconfig.out globalconfig.out.$(cfg); )

refresh:
	$(foreach cfg,$(SAMPLE_CONFIGS), \
	  cp globalconfig.out.$(cfg) globalconfig.out ; \
          CONFIG_BANNER_STRING="\"Fiasco - $(cfg)\"" make regenconfig && \
	  mv globalconfig.out globalconfig.out.$(cfg); )


%:
	$(MAKE) -I$(srcdir)  -f $(srcdir)/Makefile \
	  srcdir=$(srcdir) "$@"

.PHONY: refresh testconfig

