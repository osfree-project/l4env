ifeq (${L4_DIR},)
  L4DIR = ../..
else
  L4DIR = ${L4_DIR}
endif
include $(L4DIR)/Makeconf

SUBDIRS =	src test doc
SUBDIRS_REVERSE = $(shell j=''; for i in $(SUBDIRS); do j="$$i $$j"; done; echo $$j)

default:	$(SUBDIRS)
all:		default

$(SUBDIRS):
	@echo -e $(EMPHSTART)"Building $@"$(EMPHSTOP)
	$(VERBOSE)set -e; $(MAKE) -C $@

install:  all
	$(VERBOSE)for i in $(SUBDIRS); do $(MAKE) -C $$i install; done

clean cleanall::
	$(VERBOSE)for i in $(SUBDIRS_REVERSE); do $(MAKE) -C $$i $@; done

cleanall::
	$(VERBOSE)rm -f *~

.PHONY: $(SUBDIRS)
