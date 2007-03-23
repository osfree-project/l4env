L4DIR ?= ../..
include $(L4DIR)/mk/Makeconf

VERSION	= 0.2
CSRC	= deptrack.c syscall.c
MAN	= man/man7/libgendep.so.7
MAN_SRC	= manpage.dox Doxyfile

CFLAGS	= -fPIC -Wall -pedantic -g
DDIR	= gendep-$(VERSION)
OBJS	= $(addprefix $(OBJ_DIR)/,$(CSRC:.c=.o))
LIB	= $(OBJ_DIR)/libgendep.so
CC	= $(HOST_CC)

all:: $(LIB)

$(OBJS):: $(OBJ_DIR)/%.o: %.c
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(LIB): $(OBJS)
ifeq ($(HOST_SYSTEM),linux)
	$(HOST_CC) -shared -Wl,-soname,$@ -ldl -o $@ $^
else
ifeq ($(HOST_SYSTEM),darwin)
	$(HOST_CC) -dynamiclib -o $@ $^
else
$(error Your host system type "$(HOST_SYSTEM)" is not supported here. Fix me.)
endif
endif

doc: $(MAN)

$(MAN): $(MAN_SRC)
	doxygen

install:: $(LIB) $(MAN)
	$(if $(DROPS_STDDIR),,$(error DROPS_STDDIR is not set. Do a 'make config' in $(L4DIR)))
	$(VERBOSE)install -d $(DROPS_STDDIR)/tool/lib
	$(VERBOSE)install -c $(LIB) $(DROPS_STDDIR)/tool/lib
	$(VERBOSE)install -d $(DROPS_STDDIR)/tool/man/man7
	$(VERBOSE)install -c $(MAN) $(DROPS_STDDIR)/tool/man/man7

test:
	GENDEP_TARGET='simple-cat' \
		GENDEP_BINARY=cpp\
		GENDEP_cpp='+\.h$$ -^/usr' \
		$(LD_PRELOAD)=$(OBJ_DIR)/libgendep.so\
		$(HOST_CC) -o $(OBJ_DIR)/simple-cat simple-cat.c
	GENDEP_TARGET='blabla' \
		GENDEP_BINARY=cpp\
		$(LD_PRELOAD)=$(OBJ_DIR)/libgendep.so\
		$(HOST_CC) -o $(OBJ_DIR)/simple-cat simple-cat.c
	GENDEP_TARGET='badexp' \
		GENDEP_BINARY=cpp\
		GENDEP_cpp='\)foo'\
		$(LD_PRELOAD)=$(OBJ_DIR)/libgendep.so\
		$(HOST_CC) -o $(OBJ_DIR)/simple-cat simple-cat.c
	@echo ==========
	cat simple-cat.dep
	cat blabla.dep

clean cleanall::
	rm -f $(OBJ_DIR)/.*.d $(OBJ_DIR)/*.o $(OBJ_DIR)/simple-cat

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
