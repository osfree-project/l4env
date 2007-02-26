# source files

TEX_MAIN =	blkdev.tex
DVI_MAIN = 	$(TEX_MAIN:.tex=.dvi)
PS_MAIN =	$(TEX_MAIN:.tex=.ps)

FIGURES_FIG =	drops.fig scheduling.fig metadata.fig rpc.fig threads.fig
FIGURES_EPS =	$(FIGURES_FIG:.fig=.eps)

CLEAN_SUFFIX =	aux log toc

# tools
LATEX =		latex
DVIPS =		dvips
FIG2DEV =	fig2dev
#GV =		gv -spartan -geometry +0+0 -scale 2
#GV =           gv -spartan -geometry 792x550+0+0 -noresize
GV =            gv -spartan -geometry 950x1140+0+0 -noresize -scale 2
#GV =           gv -spartan -geometry 950x840+0+0 -noresize -scale 2
GREP =		grep
BIBTEX =	bibtex
VERBOSE = 	@

# default rule
default:	view

.PHONY:	view clean

# rules

# build DVI file. This is a bit tricky, because we need to check if we must 
# rebuild the DVI file due to references and citations.
$(DVI_MAIN): $(FIGURES_EPS) $(TEX_MAIN)
	$(LATEX) $(TEX_MAIN)
	$(VERBOSE)$(GREP) '\citation' $(DVI_MAIN:.dvi=.aux) && \
	  $(BIBTEX) $(basename $(DVI_MAIN)) || true
	$(VERBOSE)(export size=1 ; touch $(DVI_MAIN);\
	  until [ $$size -eq `ls -o $(DVI_MAIN) | awk '{print $$4}'` ]; do\
	    export size=`ls -o $(DVI_MAIN) | awk '{print $$4}'` ;\
	    $(LATEX) $(TEX_MAIN) ;\
	  done)

$(PS_MAIN): $(DVI_MAIN)
	$(DVIPS) -o $@ $<

%.eps:	%.fig
	$(FIG2DEV) -L eps -p dummy $< $*.eps

view:	$(PS_MAIN)
	$(GV) $(PS_MAIN)

clean:	
	@for i in $(CLEAN_SUFFIX); do rm -f *.$$i; done

cleanall: clean
	@rm -f $(DVI_MAIN) $(PS_MAIN) $(FIGURES_EPS) *.bak *~

