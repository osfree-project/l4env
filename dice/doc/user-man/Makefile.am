## Process this file with automake to produce Makefile.in

LATEX=          @LATEX@
BIBTEX=         @BIBTEX@
DVIPS=          @DVIPS@
PDFLATEX=       @PDFLATEX@
FIG2DEV=	@FIG2DEV@
ECHO=		echo
RM_F=          rm -f
#NUMB=		>/dev/null
NUMB=
#REAL_NUMB=	${NUMB} 2>&1
REAL_NUMB=	${NUMB}
NUMB_RM_F=	${NUMB}
NUMB_LATEX=	${REAL_NUMB}
NUMB_BIBTEX=	${NUMB}
NUMB_PDFLATEX=	${NUMB}
NUMB_DVIPS=	${REAL_NUMB}
NUMB_FIG2DEV=	${NUMB}
NUMB_GREP=	${NUMB}

TEXFILES=       manual.bib manual.tex section1.tex section2.tex \
		section3.tex appendix1.tex appendix2.tex \
		appendix3.tex commands.tex version.tex appendix4.tex
EXTRA_DIST=     $(TEXFILES)
SUFFIXES=       .tex .aux .bbl .toc .pdf .dvi .ps
CLEANFILES=     version.tex tex-stamp

ERROR_PDF=	LaTeX Warning: Label(s) may have changed. Rerun to get cross-references right.

if HAS_PDFLATEX
manual_PDF=     manual.pdf
endif

if HAS_DVIPS
manual_PS=      manual.ps
endif

if HAS_LATEX
# manual_PDF has to come before manual_PS, because pdflatex touches
# .aux files, which will make generate .ps->.dvi again, even though
# nothing changed in .tex files
all-local:      version.tex tex-stamp $(manual_PDF)

%.dvi: %.tex
	@${ECHO} Generating $*.dvi ...
	@${LATEX} $< ${NUMB_LATEX}
	@grep '\citation' `ls *.aux` ${NUMB_GREP} && ${BIBTEX} $* ${NUMB_BIBTEX} || true
	@(export size=1 ; touch $*.dvi;\
	  until [ $$size -eq `ls -o $*.dvi | awk '{print $$4}'` ]; do\
	  export size=`ls -o $*.dvi | awk '{print $$4}'` ;\
	  ${LATEX} $< ${NUMB_LATEX};\
	done)

%.pdf: %.tex
	@${ECHO} Generating $*.pdf ...
	@${PDFLATEX} $< ${NUMB_PDFLATEX}
	@${BIBTEX} $* ${NUMB_BIBTEX}
	@(while grep -q '${ERROR_PDF}' manual.log; do \
	  ${PDFLATEX} $< ${NUMB_PDFLATEX}; \
	  done)
	@${RM_F} `ls *.aux` ${NUMB_RM_F}

%.ps: %.dvi
	@${ECHO} Generating $*.ps ...
	@${DVIPS} $< -o ${NUMB_DVIPS}

%.toc: %.tex
	@${ECHO} Generating $*.toc ...
	@${LATEX} $< ${NUMB_LATEX}
	@${RM_F} $*.dvi ${NUMB_RM_F}

%.bbl: %.aux
	@${ECHO} Generating $*.bbl ...
	@${BIBTEX} $* ${NUMB_BIBTEX}
	@${LATEX} $*.tex ${NUMB_LATEX}
	@${RM_F} $*.dvi ${NUMB_RM_F}

%.aux: %.tex
	@${ECHO} Generating $*.aux ...
	@${LATEX} $< ${NUMB_LATEX}
	@${RM_F} $*.dvi ${NUMB_RM_F}

if HAS_FIG2DEV
%.eps:  %.fig
	@${FIG2DEV} -L ps -p dummy $< $*.eps ${NUMB_FIG2DEV}

endif

manual.dvi:     manual.toc manual.bbl $(TEXFILES)
manual.ps:      manual.dvi
manual.pdf:	$(TEXFILES)

endif

version.tex:
	@/bin/echo '\newcommand{\Version}{$(VERSION)}' > $@

tex-stamp: $(TEXFILES)
	@${ECHO} Generating tex-stamp ...
	@for F in $(TEXFILES); do \
	 if test ! -f $${F} -a ! -L $${F}; then \
	   ${LN_S} $(srcdir)/$${F} $${F}; \
	 fi \
	done
	@${ECHO} timestamp > tex-stamp 2>/dev/null

clean-local:
	@$(RM_F) *.dvi *.aux *.log *~ *.bbl *.blg *.toc *.ps *.pdf *.out
	@for F in $(TEXFILES); do \
          test ! -L $${F} || ${RM_F} $${F}; \
        done

