## Process this file with automake to produce Makefile.in

LATEX=          @LATEX@
BIBTEX=         @BIBTEX@
DVIPS=          @DVIPS@
PDFLATEX=       @PDFLATEX@
FIG2DEV=	@FIG2DEV@
RM_F=		rm -f
#NUMB=		>/dev/null 
NUMB=		
#REAL_NUMB=	${NUMB} 2>&1
REAL_NUMB=	${NUMB}
NUMB_RM_F= 	${NUMB}
NUMB_LATEX=	${REAL_NUMB}
NUMB_BIBTEX=	${NUMB}
NUMB_PDFLATEX=	${NUMB}
NUMB_DVIPS=	${REAL_NUMB}
NUMB_FIG2DEV=	${NUMB}
NUMB_GREP=	${NUMB}
VERBOSE=	@

TEXFILES=       dev-manual.tex version.tex commands.tex \
		message-buffer.tex communication-level.tex \
		functions.tex classes.tex parser.tex
BIBFILES=	dev-manual.bib
FIGFILES=	images/functions.fig
ALLFILES=	$(TEXFILES) $(BIBFILES) $(FIGFILES)
EXTRA_DIST=     $(ALLFILES)
SUFFIXES=       .tex .aux .bbl .toc .pdf .dvi .ps
CLEANFILES=     version.tex images/*.eps images/*.pdf

ERROR_PDF=	LaTeX Warning: Label(s) may have changed. Rerun to get cross-references right.

if HAS_PDFLATEX
manual_PDF=     dev-manual.pdf
endif

if HAS_DVIPS
manual_PS=      dev-manual.ps
endif

if HAS_LATEX
# manual_PDF has to come before manual_PS, because pdflatex touches
# .aux files, which will make generate .ps->.dvi again, even though
# nothing changed in .tex files
all-local:      version.tex  $(manual_PDF)

%.dvi:  %.tex
	$(VERBOSE)echo Generating $*.dvi ...
	$(VERBOSE)${LATEX} $< ${NUMB_LATEX}
	$(VERBOSE)grep '\citation' `ls *.aux` ${NUMB_GREP} && \
	   if [ -n "$(BIBFILES)" ]; then ${BIBTEX} $* ${NUMB_BIBTEX}; fi || true
	$(VERBOSE)(export size=1 ; touch $*.dvi;\
	  until [ $$size -eq `ls -o $*.dvi | awk '{print $$4}'` ]; do\
	  export size=`ls -o $*.dvi | awk '{print $$4}'` ;\
	  ${LATEX} $< ${NUMB_LATEX};\
	done)
									
%.pdf: %.tex
	$(VERBOSE)echo Generating $*.pdf ...
	$(VERBOSE)${PDFLATEX} $< ${NUMB_PDFLATEX}
	$(VERBOSE)(if [ -n "$(BIBFILES)" ]; then ${BIBTEX} $* ${NUMB_BIBTEX}; fi)
	$(VERBOSE)(while [ "`grep '${ERROR_PDF}' dev-manual.log`" == "${ERROR_PDF}" ]; do \
	  ${PDFLATEX} $< ${NUMB_PDFLATEX}; \
	  done)
	$(VERBOSE)${RM_F} `ls *.aux` ${NUMB_RM_F}

%.ps: %.dvi
	$(VERBOSE)echo Generating $*.ps ...
	$(VERBOSE)${DVIPS} $< -o ${NUMB_DVIPS}

%.toc: %.tex
	$(VERBOSE)echo Generating $*.toc ...
	$(VERBOSE)${LATEX} $< ${NUMB_LATEX}
	$(VERBOSE)${RM_F} $*.dvi ${NUMB_RM_F}

%.bbl: %.aux
	$(VERBOSE)echo Generating $*.bbl ...
	$(VERBOSE)(if [ -n "$(BIBFILES)" ]; then ${BIBTEX} $* ${NUMB_BIBTEX}; fi)
	$(VERBOSE)${LATEX} $*.tex ${NUMB_LATEX}
	$(VERBOSE)${RM_F} $*.dvi ${NUMB_RM_F}

%.aux: %.tex
	$(VERBOSE)echo Generating $*.aux ...
	$(VERBOSE)${LATEX} $< ${NUMB_LATEX}
	$(VERBOSE)${RM_F} $*.dvi ${NUMB_RM_F}

if HAS_FIG2DEV
%.eps:  %.fig
	$(VERBOSE)${FIG2DEV} -L ps -p dummy $< $*.eps ${NUMB_FIG2DEV}

%.pdf:  %.fig
	$(VERBOSE)${FIG2DEV} -L pdftex -p dummy $< $*.pdf ${NUMB_FIG2DEV}

endif
		
dev-manual.dvi:     dev-manual.toc dev-manual.bbl $(ALLFILES)
dev-manual.ps:      dev-manual.dvi
dev-manual.pdf:	$(ALLFILES)

$(TEXFILES): $(FIGFILES:.fig=.pdf) $(FIGFILES:.fig=.eps)

endif

version.tex:
	$(VERBOSE)echo Generating $@ ...
	$(VERBOSE)echo \\newcommand{\\Version}{${VERSION}} > $@

tex-stamp: $(ALLFILES)
	$(VERBOSE)echo Generating tex-stamp ...
	$(VERBOSE)for F in $(ALLFILES); do \
	 if test ! -f $${F} -a ! -L $${F}; then \
	   ${LN_S} $(srcdir)/$${F} $${F}; \
	 fi \
	done
	$(VERBOSE)echo timestamp > tex-stamp 2>/dev/null

clean-local:
	$(VERBOSE)$(RM_F) *.dvi *.aux *.log *~ *.bbl *.blg *.toc *.ps *.pdf
	$(VERBOSE)for F in $(ALLFILES); do \
          test ! -L $${F} || ${RM_F} $${F}; \
        done
	
