# -*- Makefile -*-
#
# DROPS (Dresden Realtime OPerating System) Component
#
# Makefile-Template for doc directories
#
# install.inc is used, see there for further documentation

ifeq ($(origin _L4DIR_MK_DOC_MK),undefined)
_L4DIR_MK_DOC_MK=y

ROLE = doc.mk
.general.d:	$(L4DIR)/mk/doc.mk

include $(L4DIR)/mk/Makeconf

# default is to install all targets
INSTALL_TARGET_MASK ?= %

ifneq ($(TARGET),)
# if no SRC_DOX is given, but TARGET, extract it from TARGET
ifeq ($(origin SRC_DOX),undefined)
SRC_DOX		   := $(filter $(addsuffix .cfg, $(TARGET)),$(wildcard *.cfg))
ifneq ($(SRC_DOX),)
$(error SRC_DOX is undefined, but TARGET is defined. This is invalid since 04/23/2003)
endif
endif
# the same for SRC_TEX
ifeq ($(origin SRC_TEX),undefined)
SRC_TEX		   := $(filter $(TARGET:.ps=.tex),$(wildcard *.tex)) \
		      $(filter $(TARGET:.dvi=.tex),$(wildcard *.tex))
ifneq ($(SRC_TEX),)
$(error SRC_TEX is undefined, but TARGET is defined. This is invalid since 04/23/2003)
endif
endif
endif

TARGET_DOX 	= $(SRC_DOX:.cfg=) $(SRC_DOX_REF:.cfg=) \
		  $(SRC_DOX_GUIDE:.cfg=) $(SRC_DOX_INT:.cfg=)
INSTALL_TARGET_DOX ?= $(filter $(INSTALL_TARGET_MASK), $(TARGET_DOX))
TARGET_TEX 	?= $(SRC_TEX:.tex=.ps)
DEPS		+= $(foreach x,$(SRC_TEX:.tex=.dvi),$(dir $x).$(notdir $x).d)

# if no TARGET is given, generate it from all types of targets
TARGET	   	?= $(TARGET_DOX) $(TARGET_TEX)
DEPS		+= $(foreach file,$(TARGET),$(dir $(file)).$(notdir $(file)).d)

all:: $(TARGET)
$(TARGET): .general.d

####################################################################
#
# Doxygen specific
#
####################################################################
DOXY_FLAGS += $(DOXY_FLAGS_$@)

# We can give an internal rule for doxygen, as the directory specified
# in the config-file should be the name of the config file with the
# .cfg removed.
%:%.cfg
        #generate the flags-file
	$(VERBOSE)$(ECHO) '@INCLUDE=$<;$(DOXY_FLAGS)' | $(TR) \; \\n >$@.flags
	$(VERBOSE)$(call MAKEDEP,doxygen) doxygen $@.flags
	$(VERBOSE)( [ -r $@/latex/Makefile ] && \
		echo | $(MAKE) -C $@/latex ) || true
	$(VERBOSE)if [ -d $@ ] ; then touch $@ ; fi

# Installation rules follow
#
# define LOCAL_INSTALLDIR prior to including install.inc, where the install-
# rules are defined. Same for INSTALLDIR.
INSTALLDIR_HTML		?= $(DROPS_STDDIR)/doc/html
INSTALLFILE_HTML	?= $(CP) -a $(1) $(2)
INSTALLDIR_HTML_LOCAL	?= $(L4DIR)/doc/html
INSTALLFILE_HTML_LOCAL	?= $(LN) -sf $(call absfilename,$(1)) $(2) && touch $(2)

INSTALLFILE		= $(INSTALLFILE_HTML)
INSTALLDIR		= $(INSTALLDIR_HTML)
INSTALLFILE_LOCAL	= $(INSTALLFILE_HTML_LOCAL)
INSTALLDIR_LOCAL	= $(INSTALLDIR_HTML_LOCAL)

all::	$(TARGET) $(addprefix $(INSTALLDIR_LOCAL)/, $(addsuffix .title, \
		$(INSTALL_TARGET_DOX)))

$(SRC_DOX_REF:.cfg=.title): BID_DOC_DOXTYPE=ref
$(SRC_DOX_GUIDE:.cfg=.title): BID_DOC_DOXTYPE=guide
$(SRC_DOX_INT:.cfg=.title): BID_DOC_DOXTYPE=int

# first line: type
# second line: title that will appear at the generated index page
%.title:%.cfg .general.d
	$(VERBOSE)$(ECHO) $(BID_DOC_DOXTYPE)>$@
	$(VERBOSE)MAKEFLAGS= $(MAKE) -s -f $(L4DIR)/mk/makehelpers.inc -f $< \
	   BID_print VAR=PROJECT_NAME >>$@

# Install the title file locally
# The installed title file depends on the installed doku for message reasons
$(foreach f,$(INSTALL_TARGET_DOX),$(INSTALLDIR_LOCAL)/$(f).title):$(INSTALLDIR_LOCAL)/%.title:%.title $(INSTALLDIR_LOCAL)/%
	$(VERBOSE)$(call INSTALLFILE_LOCAL,$<,$@)
	@$(call UPDATE_HTML_MESSAGE,$(INSTALLDIR_LOCAL))

# Install the docu locally, the title file will depend on
$(foreach f,$(INSTALL_TARGET_DOX),$(INSTALLDIR_LOCAL)/$(f)):$(INSTALLDIR_LOCAL)/%:% %/html
	@$(INSTALL_DOC_LOCAL_MESSAGE)
	$(if $(INSTALLFILE_LOCAL),$(VERBOSE)$(INSTALL) -d $@)
	$(VERBOSE)$(call INSTALLFILE_LOCAL,$</html/*,$@)

# Install the title file globally
# The installed title file depends on the installed doku for message reasons
$(foreach f,$(INSTALL_TARGET_DOX),$(INSTALLDIR)/$(f).title):$(INSTALLDIR)/%.title:%.title $(INSTALLDIR)/%
	$(VERBOSE)$(call INSTALLFILE,$<,$@)
	@$(call UPDATE_HTML_MESSAGE,$(INSTALLDIR))

# Install the docu globally, the title file will depend on
$(foreach f,$(INSTALL_TARGET_DOX),$(INSTALLDIR)/$(f)):$(INSTALLDIR)/%:% %/html
	@$(INSTALL_DOC_MESSAGE)
	$(if $(INSTALLFILE),$(VERBOSE)$(INSTALL) -d $@)
	$(VERBOSE)$(call INSTALLFILE,$</html/*,$@)

install:: $(addprefix $(INSTALLDIR)/,$(addsuffix .title,$(INSTALL_TARGET_DOX)))
.PHONY: $(addprefix $(INSTALLDIR)/,$(INSTALL_TARGET_DOX) \
				   $(addsuffix .title,$(INSTALL_TARGET_DOX)))


#################################################################
#
# Latex specific
#
#################################################################

%.eps:%.fig .general.d
	$(VERBOSE)fig2dev -L eps $< $@

%.ps:%.dvi  .general.d
	$(VERBOSE)$(call MAKEDEP,dvips) dvips -o $@ $<
	$(VERBOSE)$(VIEWERREFRESH_PS)

%.pdf:%.ps  .general.d
	ps2pdf $<

%.dvi:%.tex  .general.d
	$(VERBOSE)$(call MAKEDEP,$(LATEX)) $(LATEX) $<
	$(VERBOSE)if grep -q '\indexentry' $*.idx; then makeindex $*; fi
	$(VERBISE)if grep -q '\citation' $*.aux; then bibtex $*; fi
        # Do we really need to call latex unconditionally again? Isn't it
        # sufficient to check the logfile for the "rerun" text?
	$(VERBOSE)$(LATEX) $<
	$(VERBOSE)latex_count=5 ; \
	while egrep -s 'Rerun (LaTeX|to get cross-references right)' $*.log &&\
		[ $$latex_count -gt 0 ] ; do \
		$(LATEX) $< \
		let latex_count=$$latex_count-1 ;\
	done
	$(VERBOSE)$(VIEVERREFRESH_DVI)

SHOWTEX ?= $(firstword $(SRC_TEX))
SHOWDVI ?= $(SHOWTEX:.tex=.dvi)
SHOWPS  ?= $(SHOWTEX:.tex=.ps)

VIEWER_DVI	  ?= xdvi
VIEWER_PS         ?= gv
VIEVERREFRESH_DVI ?= killall -q -USR1 xdvi xdvi.bin xdvi.real || true
VIEWERREFRESH_PS  ?= killall -q -HUP gv || true


dvi:	$(SHOWDVI)
showdvi: dvi
	$(VERBOSE)$(VIEWER_DVI) $(SHOWDVI) &

ps:	$(SHOWPS)
showps: ps
	$(VERBOSE)$(VIEWER_PS) $(SHOWPS) &

clean::
	$(VERBOSE)$(RM) $(addsuffix .title,$(TARGET_DOX))
	$(VERBOSE)$(RM) $(addsuffix .flags,$(TARGET_DOX))
	$(VERBOSE)$(RM) $(wildcard $(foreach ext, \
		aux bbl blg dvi idx ilg ind log lod ltf toc, \
		$(SRC_TEX:.tex=.$(ext))))

cleanall:: clean
	$(VERBOSE)$(RM) -r $(wildcard $(TARGET)) $(wildcard .*.d)
	$(VERBOSE)$(RM) $(wildcard $(SRC_TEX:.tex=.ps) $(SRC_TEX:.tex=.pdf))

.PHONY: all clean cleanall config help install oldconfig reloc txtconfig
.PHONY: ps dvi showps showdvi

help::
	@echo "Specify a target:"
	@echo "all       - generate documentation and install locally"
ifneq (,$(INSTALL_TARGET_DOX))
	@echo "install   - generate documentation and install globally"
endif
	@echo "dvi       - compile the primary TeX file into dvi"
	@echo "showdvi   - invoke the dvi viewer on the primary TeX file"
	@echo "ps        - compile the primary TeX file into ps"
	@echo "showps    - invoke the ps viewer on the primary TeX file"
	@echo "clean     - delete generated intermediate files"
	@echo "cleanall  - delete all generated files"
	@echo "help      - this help"
	@echo
ifneq (,$(SRC_TEX))
	@echo "Primary TeX file: $(SHOWTEX)"
	@echo "Other documentation to be built: $(filter-out $(SHOWPS) $(SHOWDVI),$(TARGET))"
else
	@echo "Documentation to be built: $(TARGET)"
endif

-include $(DEPSVAR)

endif	# _L4DIR_MK_DOC_MK undefined
