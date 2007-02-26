L4DIR ?= ../../..

include $(L4DIR)/mk/Makeconf

SRC_TEX = signal.tex
SRC_FIG = example.fig

EPS     = $(SRC_FIG:.fig=.eps)
PDF     = $(SRC_FIG:.fig=.pdf)

all::	$(PDF)
	pdflatex $(SRC_TEX)
	pdflatex $(SRC_TEX)
	pdflatex $(SRC_TEX)

%.pdf: %.fig
	@$(GEN_MESSAGE)
	$(VERBOSE)fig2dev -L pdf $^ >$@

install:

clean::
	$(VERBOSE)$(RM) $(EPS) *.log *.aux *.dvi

cleanall: clean
	$(VERBOSE)$(RM) signal.pdf example.pdf signal.toc

show : signal.pdf
	acroread signal.pdf &
