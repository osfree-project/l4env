L4DIR ?= $(shell cd ../..; pwd)
all::
include $(L4DIR)/Makeconf.local

-include Makeconf.local

ifeq ($(wildcard ${DROPS_STDDIR}),)
$(error please specify DROPS_STDDIR using $(L4DIR)/Makeconf.local)
endif


VERSION	=0.2
CSRC	= deptrack.c syscall.c
MAN	= man/man7/libgendep.so.7
MAN_SRC	= manpage.dox Doxyfile

CFLAGS=-fPIC -Wall -pedantic -g
DDIR=gendep-$(VERSION)
OBJS= $(CSRC:.c=.o)
LIB=libgendep.so

all:: $(LIB) $(MAN)

$(LIB): $(OBJS)
	gcc -shared -Wl,-soname,$@ -o $@ $^

$(MAN): $(MAN_SRC)
	doxygen

install: $(LIB) $(MAN)
	$(VERBOSE)install -d $(DROPS_STDDIR)/tool/lib
	$(VERBOSE)install -c $(LIB) $(DROPS_STDDIR)/tool/lib
	$(VERBOSE)install -d $(DROPS_STDDIR)/tool/man/man7
	$(VERBOSE)install -c $(MAN) $(DROPS_STDDIR)/tool/man/man7

test:
	GENDEP_TARGET='simple-cat' \
		GENDEP_BINARY=cpp\
		GENDEP_cpp='+\.h$$ -^/usr' \
		LD_PRELOAD=$(shell pwd)/libgendep.so\
		gcc -o simple-cat simple-cat.c
	GENDEP_TARGET='blabla' \
		GENDEP_BINARY=cpp\
		LD_PRELOAD=$(shell pwd)/libgendep.so\
		gcc -o simple-cat simple-cat.c
	GENDEP_TARGET='badexp' \
		GENDEP_BINARY=cpp\
		GENDEP_cpp='\)foo'\
		LD_PRELOAD=$(shell pwd)/libgendep.so\
		gcc -o simple-cat simple-cat.c
	@echo ==========
	cat simple-cat.dep
	cat blabla.dep

clean cleanall::
	rm -f .*.d *.o simple-cat

cleanall::
	rm -f $(LIB)
	rm -fr man/ html/

dist:
	rm -rf $(DDIR)
	mkdir $(DDIR)
	ln $(CSRC) COPYING Doxyfile manpage.dox gendep.h Makefile \
		   WhatIsThis simple-cat.c $(DDIR)
	tar cfz $(DDIR).tar.gz $(DDIR)
	rm -rf $(DDIR)
