srcdir		:= NOT_SET
tooldir		:= $(srcdir)/../tool

CONFIG_BANNER_STRING ?= "Fiasco - prepare for world domination"


.PHONY: all do-all test-all config menuconfig xconfig oldconfig mrproper doc

ifeq ($(srcdir),NOT_SET)
all config menuconfig xconfig oldconfig:
	@echo "======================================================================"
	@echo " Building Fiasco in the src directory is deprecated!"
	@echo ""
	@echo " Go to the Fiasco root directory and create your build directory with"
	@echo "    cd .. && make BUILDDIR=build-dir"
	@echo "======================================================================"
	@exit 1

else

all: globalconfig.out globalconfig.h 

ifeq ($(filter config xconfig menuconfig oldconfig,$(MAKECMDGOALS)),)

-include globalconfig.out
CONFIG_XARCH	:= $(subst ",,$(CONFIG_XARCH))
CONFIG_ABI	:= $(subst ",,$(CONFIG_ABI))

ifeq ("$(CONFIG_XARCH)","")

all: menuconfig
	@echo "========================================================="
	@echo "Now run make again to build!"
	@echo "========================================================="
	@exit 1

else
ifeq ("$(CONFIG_ABI)","")
all:
	@echo "========================================================="
	@echo "ERROR: No ABI version set (run 'make menuconfig')!"
	@echo "========================================================="
	@exit 1
else


all: do-all

# Read Make configuration
include $(srcdir)/Makeconf
include $(srcdir)/Makerules.global

# Suck in user-specific optional Makerules file
-include Makerules.local


endif # no ABI
endif # no XARCH
endif # config xconfig menuconfig oldconfig

auto:
	test -e auto || mkdir auto

rules.out: $(srcdir)/rules.cml
	@echo =============================================
	@echo rules.out was outdated, do \'make menuconfig\'
	@echo to see if there are new config options!
	@echo =============================================
	$(tooldir)/cml2/cmlcompile.py -o $@ $<

globalconfig.out: rules.out
	$(tooldir)/cml2/cmlconfigure.py -b -B $(CONFIG_BANNER_STRING) -c -i globalconfig.out -o globalconfig.out $<

globalconfig.h: globalconfig.out
	$(tooldir)/cml2/configtrans.py -h $@ $<

config menuconfig: rules.out
	$(tooldir)/cml2/cmlconfigure.py -B $(CONFIG_BANNER_STRING) -c -i globalconfig.out -o globalconfig.out $< && \
	$(tooldir)/cml2/configtrans.py -h globalconfig.h globalconfig.out

oldconfig: rules.out
	$(tooldir)/cml2/cmlconfigure.py -B $(CONFIG_BANNER_STRING) -t -I globalconfig.out -o globalconfig.out $< && \
	$(tooldir)/cml2/configtrans.py -h globalconfig.h globalconfig.out

xconfig: rules.out
	$(tooldir)/cml2/cmlconfigure.py -x -i globalconfig.out -o globalconfig.out $< && \
	$(tooldir)/cml2/configtrans.py -h globalconfig.h globalconfig.out

clean: $(foreach subsys, $(SUBSYSTEMS), clean-$(subsys))
	rm -f $(ALL)
	rm -f *.o
	rm -f auto/*.cc auto/*.h auto/*.S auto/stamp-*.ready

cleanall: clean $(foreach subsys, $(SUBSYSTEMS), cleanall-$(subsys))
	$(foreach subdir, $(SUBDIRS), rm -f $(subdir)/{.,}*.d)
	rm -f tcboffset.h
	rm -f {.,}*.d *~
	rm -f .Modules.deps

mrproper: cleanall
	rm -rf globalconfig.* Modules rules.out auto
	rm -rf $(srcdir)/Modules $(srcdir).Modules.deps $(srcdir)/auto 
	rm -rf $(srcdir)/globalconfig.h $(srcdir)/globalconfig.out $(srcdir)/rules.out
	rm -rf docs
endif
