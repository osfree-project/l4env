#
# $Id$
#
# Makefile for the manpages of the "hello world" test server
#

# scan the L4 global configuration file
ifeq (${L4_DIR},)
  L4DIR = ../../../..
else
  L4DIR = ${L4_DIR}
endif
include $(L4DIR)/Makeconf

-include Makeconf.local			# read possible PRIVATE_DIRS

# enviroment
SRC = $(wildcard *.sgml)

#default make action
all: man
man:		$(SRC:.sgml=.man)

everything:	man html ps

html:		$(SRC:.sgml=.html)

ps:		$(SRC:.sgml=.ps)

install:	man
	@echo "Installing Manuals..."
	$(INSTALL) -d $(L4_INSTALL_DIR)/man/man1/
	$(INSTALL) -m 644 hello.man $(L4_INSTALL_DIR)/man/man1/

.SUFFIXES:	.sgml .man .html .ps

.sgml.man:
	sdc -Oman -o$*.man $*.sgml

.sgml.html:
	sdc -Ohtml -o$*.html $*.sgml

.sgml.ps:
	sdc -Ops -o$*.ps $*.sgml

clean:
	$(CLEAN_MESSAGE)
	@rm -fv *.html *.ps

cleanall:	clean
	@rm -fv *~
