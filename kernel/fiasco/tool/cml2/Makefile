#
# Makefile for the CML2 build system
#

# Packaging

PYTHON   = cmladvent.py cmlcompile.py cmlconfigure.py tree.py \
	cml.py cmlsystem.py configtrans.py helpmerge.py kxref.py \
	autoconfigure.py autoconfigure.rules
SCRIPTS  = install-cml2 helpmerge.py # find-upgrades
DOCS     = READ.ME READ.ME.LINUX COPYING ANNOUNCEMENT NEWS TODO cml2-paper.sgml cml2-reference.sgml torture.test
FILES    = Makefile $(DOCS) kernel-tree $(PYTHON) $(SCRIPTS)

VERSION=$(shell cmlconfigure.py -V | sed "s/cmlconfigure //")

EXCLUDES = --exclude "*RCS*" --exclude "*py[co]" --exclude "*rules.out" \
		--exclude "*,v" --exclude "*~" 

dist: cml2-$(VERSION).tar.gz 

cml2-$(VERSION).tar.gz: $(FILES)
	@mkdir cml2-$(VERSION); cp $(FILES) cml2-$(VERSION); cp -r kernel-tree cml2-$(VERSION)
	tar $(EXCLUDES) -czf cml2-$(VERSION).tar.gz cml2-$(VERSION)
	@(rm -fr cml2-$(VERSION))

version:
	@echo $(VERSION)

# CML2 documentation -- these require docbook-tools and jade.

cml2-paper.html: cml2-paper.sgml
	docbook2html --nochunks cml2-paper.sgml >cml2-paper.html

cml2-reference.html: cml2-reference.sgml
	docbook2html --nochunks cml2-reference.sgml >cml2-reference.html

cml2-paper.ps: cml2-paper.sgml
	docbook2ps cml2-paper.sgml

cml2-reference.ps: cml2-reference.sgml
	docbook2ps cml2-reference.sgml

# Running pychecker

check:
	checker.py *.py

# Freezing the code to an executable binary.
# Note: this requires your Python library to be statically linked!

FREEZECMD = /usr/bin/python freezetools/freeze.py
freeze:
	-mkdir c-cmlcompile
	$(FREEZECMD) -o c-cmlcompile cmlcompile.py >c-cmlcompile/FREEZELOG 2>&1
	(cd c-cmlcompile; make)
	-mkdir c-cmlconfigure
	$(FREEZECMD) -o c-cmlconfigure cmlconfigure.py >c-cmlconfigure/FREEZELOG 2>&1
	(cd c-cmlconfigure; make)
	cp c-cmlcompile/cmlcompile .
	cp c-cmlconfigure/cmlconfigure .

melt:
	rm -fr c-cmlcompile c-cmlconfigure

# Utility:

arch:
	@echo $(ARCH)

clean:
	rm -f rules.out *.pyc *~ typescript config.h config.sh
	rm -fr c-cmlcompile/ c-cmlconfigure/ cml2-paper/ cml2-reference/
	rm -fr cml2-*.html cml2-*.aux cml2-*.dvi cml2-*.tex cml2-*.log cml2-*.junk/

### That's all, folks!
