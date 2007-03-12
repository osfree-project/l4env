# directories we need to know
PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

FILES  = $(PKGDIR_ABS)/common/fileops.h \
	 $(PKGDIR_ABS)/lib/audio/lame/lame.h \
	 $(PKGDIR_ABS)/lib/audio/wave.h \
	 $(PKGDIR_ABS)/lib/audio/mpg123lib/mpglib.h \
	 $(PKGDIR_ABS)/lib/audio/mpg123lib/mpg123.h \
	 $(PKGDIR_ABS)/lib/support/aclib/aclib.h \
	 $(PKGDIR_ABS)/lib/support/x86cpucaps/x86cpucaps.h \
	 $(PKGDIR_ABS)/lib/video/libavcodec/avcodec.h \
	 $(PKGDIR_ABS)/lib/video/postproc/postprocess.h \
	 $(PKGDIR_ABS)/lib/video/xvidcore-1.0/src/xvid.h \
	 $(PKGDIR_ABS)/lib/video/libmpeg2/mpeg2.h \
	 $(PKGDIR_ABS)/lib/de-muxer/avilib/avilib.h \
	 $(PKGDIR_ABS)/lib/de-muxer/ogmlib/ogmlib.h \
	 $(PKGDIR_ABS)/lib/de-muxer/ogmlib/ogmstreams.h \
	 $(PKGDIR_ABS)/lib/de-muxer/probe/probe_mp3.h \
	 $(PKGDIR_ABS)/lib/de-muxer/probe/probe_ogm.h \
	 $(PKGDIR_ABS)/lib/de-muxer/probe/probe_wav.h \
	 $(PKGDIR_ABS)/lib/de-muxer/probe/probe_mpeg.h \
	 $(PKGDIR_ABS)/lib/de-muxer/probe/probe_avi.h \
	 $(PKGDIR_ABS)/lib/fx/goom/goom.h \
	 $(PKGDIR_ABS)/lib/predict/predict.h

TARGETS = $(addprefix $(OBJ_DIR)/,$(notdir $(FILES)))

all:: $(TARGETS)

$(TARGETS):
	$(VERBOSE)cd $(OBJ_DIR) && for i in $(subst $(L4DIR_ABS),$(L4DIR)/source,$(FILES)); do ln -sf $${i} `basename $${i}`; done

clean cleanall:
	$(VERBOSE)$(RM) -f $(addprefix $(OBJ_DIR)/,$(notdir $(FILES)))

install:
