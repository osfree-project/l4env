# This makefile is a template for a build in a directory other 
# than src. 
# The only thing you have to do is to copy this template to
# your build dir (cp Makefile.builddir.templ <builddir>/Makefile)
# and change srcdir to the relative path to your fiasco/src.
#

SAMPLE_CONFIGS := $(subst globalconfig.out.,,$(wildcard globalconfig.out.*))

srcdir = ../

all:

testconfig:
	$(foreach cfg,$(SAMPLE_CONFIGS), \
	  cp globalconfig.out.$(cfg) globalconfig.out ; \
          CONFIG_BANNER_STRING="\"Fiasco - $(cfg)\"" make menuconfig && \
	  mv globalconfig.out globalconfig.out.$(cfg); )

refresh:
	for cfg in $(SAMPLE_CONFIGS); do                                 \
	  cp globalconfig.out.$$cfg globalconfig.out;                    \
          CONFIG_BANNER_STRING="\"Fiasco - $(cfg)\"" make regenconfig && \
	  sed -e '/^# Generated on: /d' -e '/^# Linux version /d '       \
	      -e '/^# At: /d' globalconfig.out > globalconfig.out.$$cfg; \
	  $(RM) globalconfig.out;                                        \
	done
	$(RM) rules.out


%:
	$(MAKE) -I$(srcdir)  -f $(srcdir)/Makefile \
	  srcdir=$(srcdir) objbase=. "$@"

.PHONY: refresh testconfig

