#
# $Id$
#
# GLOBAL Makefile for the whole L4 tree
#

ifeq (${OSKIT_DIR},)
  OSKITDIR = ../oskit
else
  OSKITDIR = ${OSKIT_DIR}
endif

SUBDIRS = pkg kernel 
#doc

HELLODIRS =	pkg/l4sys/ \
		pkg/l4util/ \
		pkg/l4dbg/ \
		pkg/crtx/ \
		pkg/rmgr/ \
		pkg/oskit_support/ \
		pkg/sigma0/ \
		pkg/hello/

default: all

all:: oskit

.PHONY: all clean cleanall

cleanall::
	rm -f *~

all clean cleanall::
	for i in $(SUBDIRS) ; do \
	    $(MAKE) -C $$i $@ ; \
	  done

$(SUBDIRS):
	$(MAKE) -C $@

# build 'Hello World' Server
hello:	oskit
	cd pkg && $(MAKE) pkg-links
	set -e; for i in $(HELLODIRS) ; do \
	  $(MAKE) -C $$i lib ; \
	done
	set -e; for i in $(HELLODIRS) ; do \
	  $(MAKE) -C $$i ; \
	done

# OSKIT rules

.PHONY: oskit oskit.conf oskit.present $(SUBDIRS)

oskit: oskit.present oskit.conf
	cd $(OSKITDIR) && $(MAKE)

oskit.present:
	@cd $(OSKITDIR) || \
	  (echo "ERROR: please install OSKIT to $(OSKITDIR)"; exit 1)

oskit.conf: $(OSKITDIR)/GNUmakefile

$(OSKITDIR)/GNUmakefile:
	cd $(OSKITDIR) && ./configure
