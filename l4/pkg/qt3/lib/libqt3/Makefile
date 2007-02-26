PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

# hmm. $(GCCINCDIR) is undefined if this is not set
SYSTEMS = x86
ARCH    = x86

ifeq ($(USE_UCLIBC),y)
LIBC_INCDIR  = $(L4DIR)/include/x86/uclibc \
	       $(L4DIR)/include/uclibc \
	       $(DROPS_STDDIR)/include/x86/uclibc \
	       $(DROPS_STDDIR)/include/uclibc
else
LIBC_INCDIR =  $(L4DIR)/include/dietlibc \
	       $(DROPS_STDDIR)/include/dietlibc
endif

# use these include dirs for compilation of the Qt library
L4INCDIRS_QT = $(call absfilename,$(LIBC_INCDIR) \
				  $(L4DIR)/include/l4/libpng \
				  $(L4DIR)/include/l4/zlib) \
		$(GCCINCDIR)

# DROPS-specific platform configuration
QMAKECONF    = mkspecs/qws/drops-x86-g++/qmake.conf


CONTRIB_PREP_REQ = $(QMAKECONF).template Makefile src/tools/qconfig-drops.h

# let's go
all:: drops-install

# configure Qt build process
contrib-prep: $(CONTRIB_PREP_REQ)
	$(VERBOSE)$(MAKE) cleanall
	$(VERBOSE)./scripts/make-qmake-conf.sh $(QMAKECONF) \
	     $(CC_$(ARCH)) $(CXX_$(ARCH)) "$(L4INCDIRS_QT)"
	$(VERBOSE)echo -n "Creating links from ../contrib/qt-embedded-free/ ... "
	$(VERBOSE)./scripts/make-links.sh .
	$(VERBOSE)./scripts/restore-include-links.sh
	$(VERBOSE)./scripts/restore-mkspecs-links.sh
	$(VERBOSE)echo "done"
	$(VERBOSE)echo -e 'yes\n\n' | \
	    ./configure \
		-static -debug -no-exceptions -qconfig drops \
		-xplatform qws/drops-x86-g++ -thread -depths 16,24,32 \
		-no-stl -no-largefile -no-ipv6 -no-cups \
		-no-kbd-driver -no-mouse-driver -no-gfx-snap \
		-disable-opengl \
		-qt-gif -no-freetype -qt-libjpeg -qt-libmng \
		-system-zlib -qt-libpng
	$(VERBOSE)touch contrib-prep

# build library
.PHONY: contrib-build
contrib-build: contrib-build-lib

.PHONY: contrib-build-moc
contrib-build-moc: contrib-prep contrib-links-src 
	$(VERBOSE)$(MAKE) -f Makefile.qt src-moc

.PHONY: contrib-build-lib
contrib-build-lib: contrib-build-moc
	$(VERBOSE)$(MAKE) -f Makefile.qt sub-src

# recreate links in directories, which may contain source files
.PHONY: contrib-links-src
contrib-links-src:
	$(VERBOSE)echo -n "Recreating links from ../contrib/qt-embedded-free/ ... "
	$(VERBOSE)./scripts/make-links.sh mkspecs
	$(VERBOSE)./scripts/restore-mkspecs-links.sh
	$(VERBOSE)./scripts/make-links.sh qmake
	$(VERBOSE)./scripts/make-links.sh plugins
	$(VERBOSE)./scripts/make-links.sh src
	$(VERBOSE)echo "done"

# install headers and libqt3_embedded.a into L4 tree
.PHONY: drops-install
drops-install: contrib-build
	$(VERBOSE)$(MAKE) -C include
	$(VERBOSE)$(MAKE) -C lib

clean::
	$(VERBOSE)$(MAKE) -f Makefile.qt clean || true

cleanall::
	$(VERBOSE)$(MAKE) -C include cleanall
	$(VERBOSE)$(MAKE) -C lib cleanall
	$(VERBOSE)rm -f contrib-prep
	$(VERBOSE)$(MAKE) -f Makefile.qt confclean || true
	$(VERBOSE)cd qmake && $(MAKE) distclean || true
	$(VERBOSE)cd src/moc && $(MAKE) distclean || true
	$(VERBOSE)rm -rf config.tests
	$(VERBOSE)find . -type d -a \( -name ".moc" -o -name ".obj" \) | xargs rm -rf
	$(VERBOSE)find . -type l | xargs rm -f
	$(VERBOSE)find . -name ".make-links*" | xargs rm -f

install::
	$(VERBOSE)$(MAKE) -C lib install
	$(VERBOSE)$(MAKE) -C include install

reloc::
	#

include $(L4DIR)/mk/Makeconf

