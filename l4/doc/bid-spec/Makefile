-include Makeconf.local

.DELETE_ON_ERROR:

SRC_TEX = spec.tex
ABSTRACT_TEX = abstract.tex
HTML_TEX = html.tex
LANG_TEX = config-language.tex
LANG_FILE = config-language.txt
SHOW = spec

VIEW_PS = $(SHOW).ps
VIEW_DVI = $(SHOW).dvi

DVI = $(SHOW).dvi
PS = $(SHOW).ps
PDF = $(SHOW).pdf
ABSTRACT = $(ABSTRACT_TEX:.tex=.ps)

all::
show run: seedvi

SRC_FIG = $(wildcard *.fig)
EPS = $(SRC_FIG:.fig=.eps)

LAFIG = $(wildcard *.lafig)
LTF = $(patsubst %.lafig,%.ltf,$(LAFIG))


BIB = $(wildcard bib/*.bib *.bib)

LATEX = latex -interaction=batchmode

html:	.html

.html:	$(SRC_TEX) Makefile $(LANG_FILE) $(LANG_TEX)
	rm -rf $(HTML_TEX:.tex=)
	sed -e 's/«/\\begin{code}/g' -e 's/»/\\end{code}\\rm{}/g' \
	    -e 's/§/\\$$/g' -e 's/\\#/\\\\\\hline/g' \
            <$(SRC_TEX) >$(HTML_TEX) && \
	  cp $(SRC_TEX:.tex=.aux) $(HTML_TEX:.tex=.aux)
	  cp $(SRC_TEX:.tex=.bbl) $(HTML_TEX:.tex=.bbl)
	  latex2html -local_icons -split 5 -show_section_numbers \
		$(HTML_TEX) && \
	  true rm $(HTML_TEX) $(HTML_TEX:.tex=.aux) && cp $(LANG_FILE) html/
	  touch .html

%.ltf: %.lafig
	fig2dev -L latex $< $@

%.bbl:  %.tex
	rm $@

%.eps:	%.fig
	fig2dev -L eps $< $*.eps

%.dvi:	%.tex
	$(LATEX) $<
	@echo -e "\033[34mMaking $@ because of $?\033[0m"
	grep -q '\citation' $*.aux && bibtex $* || true
	grep -q '\indexentry' $*.idx && makeindex $* || true
	@(export size=1 ; touch $*.dvi;\
	 until [ $$size -eq `ls -o $*.dvi | awk '{print $$4}'` ]; do\
	   export size=`ls -o $*.dvi | awk '{print $$4}'` ;\
	   $(LATEX) $< ;\
	 done)
	killall -USR1 xdvi xdvi.bin xdvi.real || true

%.ps:	%.dvi
	dvips -o $@ $<

%.pdf:	%.ps
	ps2pdf $<

seedvi:: $(VIEW_DVI)
	xdvi  $(xdvi_big_flags) $^ &

seeps:: $(VIEW_PS)
	gv $^

seebw:: $(VIEW_PS)
	gv -magstep -2 -grayscale -display $(DISP) $^

ifeq ($(HTMLDIR),)
pub::
	@echo 'Specify your http server directory in HTMLDIR in Makeconf.local'
	@echo 'Then, "make pub" will copy all the files into this directory'
	exit 1
else
pub:: $(PS) .html $(PDF)
	install -d $(HTMLDIR)
	install html/* -m 664 $(HTMLDIR)
	install $(PS) $(PDF) -m 644 $(HTMLDIR)
endif

all:: $(PS) $(ABSTRACT) $(ABSTRACT)

clean::
	@rm -fv *.aux *.log *.toc *.bbl *.blg *.ltf *.lod *.idx *.ilg *.ind
	@rm -fv k.*
	@for i in *.fig; do\
		j=`basename $$i .fig`.eps; \
		rm -vf $$j;\
	done
	@rm -frv html .html

cleanall:: clean
	@rm -fv *.ps *.dvi *.pdf

help::
	@echo "Specify a target:"
	@echo "all      - build postscript version (default)"
	@echo "html     - build html version"
	@echo "pub      - build html version and copy it to the http server"
	@echo "           this requires \"HTMLDIR\" to be set in Makeconf.local"
	@echo "help     - this help"

$(DVI): $(SRC_TEX) $(LANG_TEX) $(EPS) $(BIB) $(LTF)
abstract.dvi: abstract.tex $(BIB)
