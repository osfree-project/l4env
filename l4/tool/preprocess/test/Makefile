# scan the L4 global configuration file
L4DIR	?= ../../..
include $(L4DIR)/mk/Makeconf

TOOL = ../src/preprocess

all:: test

ALL_TESTS = $(TOOL_TESTS)
TOOL_TESTS = mapping mapping_inline random random_inline extern_c static \
  noinline explicit operator template template_inline c-preproc inline \
  parser parser_noinline \
  multifile variable line line_not line_nh interface comment_in_string \
  default_args

mapping_inline_src = mapping
mapping_inline_flags = -i

random_inline_src = random
random_inline_flags = -i

static_flags = -i

inline_flags = -i

noinline_flags = -i

template_inline_src = template
template_inline_flags = -i

parser_flags = -i

parser_noinline_src = parser
parser_noinline_flags = 

multifile_src = multifile1 multifile2
multifile_flags = -i
multifile_extra = multifile-part1.cc multifile-part2.cc

line_not_src = line
line_not_flags = -l

line_nh_src = line
line_nh_flags = -L

interface_missing = interface.h
interface_extra = interfacepublic.h

random.cpp: combine.pl
	perl $< > $@.new
	mv $@.new $@

clean::
	rm -f random.cpp

test_rules: Makefile
	rm -f $@.new
	for test in $(TOOL_TESTS); \
	  do \
	    echo "ifndef $${test}_src" >> $@.new; \
	    echo "$${test}_src = $${test}" >> $@.new; \
	    echo "endif" >> $@.new; \
	    echo "ifndef $${test}_flags" >> $@.new; \
	    echo "$${test}_flags = " >> $@.new; \
	    echo "endif" >> $@.new; \
	    echo "$${test}.cc: \$$(addsuffix .cpp, \$$($${test}_src)) \$$(TOOL); \$$(TOOL) \$$($${test}_flags) -c $${test} \$$(filter-out \$$(TOOL), \$$^)" >> $@.new; \
	  done
	mv $@.new $@

include test_rules

clean::
	rm -f $(addsuffix .cc, $(TOOL_TESTS)) \
	  $(addsuffix .h, $(TOOL_TESTS)) \
	  $(addsuffix _i.h, $(TOOL_TESTS)) 

.PHONY: test
test: $(addsuffix .t.ok, $(ALL_TESTS)) 

%.t.ok: %.cc
	@echo -n "Running test $* ... "
ifeq ($(RECREATE_OUTPUT),1)
	@cp $(filter-out $($*_missing),$*.h $*_i.h $*.cc) $($*_extra) verify/
endif
	@for i in $(filter-out $($*_missing),$*.h $*_i.h $*.cc) $($*_extra); \
	  do \
	    diff -u verify $$i || exit 1; \
	  done
	@echo "OK"
	@touch $@

.PHONY: init
init:
	$(MAKE) test RECREATE_OUTPUT=1

install:
	@echo Not installing tests.

clean:: 
	$(RM) $(ALL) $(OBJS) *.t.ok

cleanall::      clean
	$(RM) *~ .*.d test_rules

#include $(DEPS)
