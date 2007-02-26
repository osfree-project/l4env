#
# Files
#

DOC_TEX =	building.tex

#
# Rules and stuff 
# 

default:	doc_ps

# Tools
LATEX =		latex
DVIPS =		dvips
FIG2DEV =	fig2dev
#GV =		gv -spartan -geometry 950x1140+0+0 -noresize -scale 2
#GV =		gv -spartan -geometry 950x845+0+0 -noresize -scale 2
##GV =		gv -spartan -geometry 790x550+0+0 -noresize -antialias
GREP = 		grep
RM =		rm -f

DOC_DVI =	$(DOC_TEX:.tex=.dvi)
DOC_PS =	$(DOC_TEX:.tex=.ps)

DOC_FIGURES_EPS =	$(DOC_FIGURES:.fig=.eps)

DOC_CLEAN =	$(DOC_DVI) $(DOC_FIGURES_EPS) \
		*.aux *.log *.toc *.bbl *.blg *.ltf

VERBOSE = @

# build DVI file. This is a bit tricky, because we need to check if we must 
# rebuild the DVI file due to references and citations.
$(DOC_DVI): $(DOC_FIGURES_EPS) $(DOC_TEX) $(DOC_TEX_ADD) $(DOC_BIB)
	$(VERBOSE)$(LATEX) $(DOC_TEX)
	$(VERBOSE)$(GREP) '\citation' $(DOC_DVI:.dvi=.aux) && \
	  bibtex $(basename $(DOC_DVI)) || true
	$(VERBOSE)(export size=1 ; touch $(DOC_DVI);\
	  until [ $$size -eq `ls -o $(DOC_DVI) | awk '{print $$4}'` ]; do\
	    export size=`ls -o $(DOC_DVI) | awk '{print $$4}'` ;\
	    $(LATEX) $(DOC_TEX) ;\
	  done)

$(DOC_PS): $(DOC_DVI)
	$(VERBOSE)$(DVIPS) -o $@ $<

doc_ps: $(DOC_PS)

doc_view: $(DOC_PS)
	$(VERBOSE)$(GV) $(DOC_PS)

clean cleanall::
	$(VERBOSE)$(RM) $(DOC_CLEAN)

cleanall::
	$(VERBOSE)$(RM) $(DOC_PS)
	$(VERBOSE)$(RM) -r $(DOC_HTML_DIR)

%.eps:  %.fig
	$(FIG2DEV) -L ps -p dummy $< $*.eps
