
# scan the L4 global configuration file
ifeq (${L4_DIR},)
  L4DIR = ../../..
else
  L4DIR = ${L4_DIR}
endif
include $(L4DIR)/Makeconf

.SUFFIXES: .sgml .html .man

SRC=$(wildcard *.sgml)
MAN=$(SRC:.sgml=.man)
HTML=$(SRC:.sgml=.html)

all: man

man: $(MAN)
html: $(HTML)

$(MAN) $(HTML): docchars.ent

.sgml.man:    
	sdc -Oman -o$*.man $*.sgml

.sgml.html:    
	sdc -Ohtml -o$*.html $*.sgml

clean cleanall:
	rm -f $(MAN) $(HTML) *~

install:        man
	@echo "Installing Manuals..."
	$(INSTALL) -d $(L4_INSTALL_DIR)/man/man1
	$(INSTALL) -m 644 $(MAN) $(L4_INSTALL_DIR)/man/man1
