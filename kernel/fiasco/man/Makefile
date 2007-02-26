#
# fiasco kernel manpage
#

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

clean:
	rm -f $(HTML)

cleanall: clean
	rm -f *~
