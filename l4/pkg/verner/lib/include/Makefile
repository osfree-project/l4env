# directories we need to know
PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

FILES  = $(PKGDIR)/common/fileops.h \
	 $(PKGDIR)/lib/audio/lame/lame.h \
	 $(PKGDIR)/lib/audio/wave.h \
	 $(PKGDIR)/lib/audio/mpg123lib/mpglib.h \
	 $(PKGDIR)/lib/audio/mpg123lib/mpg123.h \
	 $(PKGDIR)/lib/support/libc/cr7libc.h \
	 $(PKGDIR)/lib/support/aclib/aclib.h \
	 $(PKGDIR)/lib/support/x86cpucaps/x86cpucaps.h \
	 $(PKGDIR)/lib/video/libavcodec/avcodec.h \
	 $(PKGDIR)/lib/video/libavcodec/common.h \
	 $(PKGDIR)/lib/video/postproc/postprocess.h \
	 $(PKGDIR)/lib/video/xvidcore-0.9.1/src/xvid.h \
	 $(PKGDIR)/lib/video/libmpeg2/mpeg2.h \
	 $(PKGDIR)/lib/de-muxer/avilib/avilib.h \
	 $(PKGDIR)/lib/de-muxer/ogmlib/ogmlib.h \
	 $(PKGDIR)/lib/de-muxer/ogmlib/ogmstreams.h \
	 $(PKGDIR)/lib/de-muxer/probe/probe_mp3.h \
	 $(PKGDIR)/lib/de-muxer/probe/probe_ogm.h \
	 $(PKGDIR)/lib/de-muxer/probe/probe_wav.h \
	 $(PKGDIR)/lib/de-muxer/probe/probe_mpeg.h \
	 $(PKGDIR)/lib/de-muxer/probe/probe_avi.h \
	 $(PKGDIR)/lib/fx/goom/goom.h \
	 $(PKGDIR)/lib/predict/predict.h

all:: $(notdir $(FILES)) xvid_10.h

$(notdir $(FILES)):
	$(VERBOSE)for i in $(FILES); do ln -sf $${i} `basename $${i}`; done

xvid_10.h: $(PKGDIR)/lib/video/xvidcore-1.0/src/xvid.h
	$(VERBOSE)$(LN) -sf $< $@

clean cleanall:
	$(VERBOSE)$(RM) $(notdir $(FILES)) xvid_10.h

install:
