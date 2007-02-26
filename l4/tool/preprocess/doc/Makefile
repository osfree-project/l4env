ifeq (${L4_DIR},)
  L4DIR = ../../..
else
  L4DIR = ${L4_DIR}
endif
include $(L4DIR)/Makeconf

ALL = html man
all: $(foreach d, $(ALL), stamp-$(d))

DIRS = html man latex
.PHONY: $(DIRS)
$(DIRS) : % : stamp-%

stamp-%: Doxyfile manpage.dox
	( echo "ENABLED_SECTIONS = $*"; \
	  for format in latex html man; do \
	    echo "GENERATE_$$(echo $$format | tr a-z A-Z) = \
	          $$(if [ $$format = $* ]; then echo YES; else echo NO; fi)"; \
	  done; \
	  cat $<) | doxygen -
	if [ "$*" = html ]; then \
	  perl -p -i -e 's/preprocess.html#/#/g' html/preprocess.html; \
	fi
	touch $@

install: stamp-man
	-$(INSTALL) -d $(L4_INSTALL_DIR)/man/man1
	$(INSTALL) -m 644 man/man1/preprocess.1 \
	  $(L4_INSTALL_DIR)/man/man1/preprocess.1


cleanall clean:
	rm -rf $(DIRS) stamp-*
