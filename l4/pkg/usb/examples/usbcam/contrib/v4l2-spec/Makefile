# Also update in v4l2.sgml!
VERSION = 0.25

SGMLS = \
	biblio.sgml \
	capture.c.sgml \
	common.sgml \
	compat.sgml \
	controls.sgml \
	dev-capture.sgml \
	dev-codec.sgml \
	dev-effect.sgml \
	dev-osd.sgml \
	dev-output.sgml \
	dev-overlay.sgml \
	dev-radio.sgml \
	dev-raw-vbi.sgml \
	dev-rds.sgml \
	dev-sliced-vbi.sgml \
	dev-teletext.sgml \
	driver.sgml \
	entities.sgml \
	fdl-appendix.sgml \
	func-close.sgml \
	func-ioctl.sgml \
	func-mmap.sgml \
	func-munmap.sgml \
	func-open.sgml \
	func-poll.sgml \
	func-read.sgml \
	func-select.sgml \
	func-write.sgml \
	indices.sgml \
	io.sgml \
	pixfmt-grey.sgml \
	pixfmt-nv12.sgml \
	pixfmt-nv16.sgml \
	pixfmt-packed-rgb.sgml \
	pixfmt-packed-yuv.sgml \
	pixfmt-sbggr16.sgml \
	pixfmt-sbggr8.sgml \
	pixfmt-sgbrg8.sgml \
	pixfmt-uyvy.sgml \
	pixfmt-vyuy.sgml \
	pixfmt-y16.sgml \
	pixfmt-y41p.sgml \
	pixfmt-yuv410.sgml \
	pixfmt-yuv411p.sgml \
	pixfmt-yuv420.sgml \
	pixfmt-yuv422p.sgml \
	pixfmt-yuyv.sgml \
	pixfmt-yvyu.sgml \
	pixfmt.sgml \
	v4l2.sgml \
	videodev2.h.sgml \
	vidioc-cropcap.sgml \
	vidioc-dbg-g-register.sgml \
	vidioc-encoder-cmd.sgml \
	vidioc-enum-fmt.sgml \
	vidioc-enum-frameintervals.sgml \
	vidioc-enum-framesizes.sgml \
	vidioc-enumaudio.sgml \
	vidioc-enumaudioout.sgml \
	vidioc-enuminput.sgml \
	vidioc-enumoutput.sgml \
	vidioc-enumstd.sgml \
	vidioc-g-audio.sgml \
	vidioc-g-audioout.sgml \
	vidioc-dbg-g-chip-ident.sgml \
	vidioc-g-crop.sgml \
	vidioc-g-ctrl.sgml \
	vidioc-g-enc-index.sgml \
	vidioc-g-ext-ctrls.sgml \
	vidioc-g-fbuf.sgml \
	vidioc-g-fmt.sgml \
	vidioc-g-frequency.sgml \
	vidioc-g-input.sgml \
	vidioc-g-jpegcomp.sgml \
	vidioc-g-modulator.sgml \
	vidioc-g-output.sgml \
	vidioc-g-parm.sgml \
	vidioc-g-priority.sgml \
	vidioc-g-sliced-vbi-cap.sgml \
	vidioc-g-std.sgml \
	vidioc-g-tuner.sgml \
	vidioc-log-status.sgml \
	vidioc-overlay.sgml \
	vidioc-qbuf.sgml \
	vidioc-querybuf.sgml \
	vidioc-querycap.sgml \
	vidioc-queryctrl.sgml \
	vidioc-querystd.sgml \
	vidioc-reqbufs.sgml \
	vidioc-s-hw-freq-seek.sgml \
	vidioc-streamon.sgml \

PDFPICS = \
	crop.pdf \
	fieldseq_bt.pdf \
	fieldseq_tb.pdf \
	vbi_525.pdf \
	vbi_625.pdf \
	vbi_hsync.pdf

GIFPICS = \
	crop.gif \
	fieldseq_bt.gif \
	fieldseq_tb.gif \
	vbi_525.gif \
	vbi_625.gif \
	vbi_hsync.gif

# Some Jade warnings for authors.
WARN =
#WARN = -w all -w duplicate -w net

# Stylesheet customization.
CUSTOM_HTML = --dsl custom.dsl\#html
CUSTOM_PRINT = --dsl custom.dsl\#print

all: capture html-single

html: html-build.stamp

html-single: html-single-build.stamp

pdf: pdf-build.stamp

coffeebreak: all html pdf

FUNCS = \
	close \
	ioctl \
	mmap \
	munmap \
	open \
	poll \
	read \
	select \
	write \

IOCTLS = \
	VIDIOC_CROPCAP \
	VIDIOC_DBG_G_CHIP_IDENT \
	VIDIOC_DBG_G_REGISTER \
	VIDIOC_DBG_S_REGISTER \
	VIDIOC_DQBUF \
	VIDIOC_ENCODER_CMD \
	VIDIOC_ENUMAUDIO \
	VIDIOC_ENUMAUDOUT \
	VIDIOC_ENUMINPUT \
	VIDIOC_ENUMOUTPUT \
	VIDIOC_ENUMSTD \
	VIDIOC_ENUM_FMT \
	VIDIOC_ENUM_FRAMEINTERVALS \
	VIDIOC_ENUM_FRAMESIZES \
	VIDIOC_G_AUDIO \
	VIDIOC_G_AUDOUT \
	VIDIOC_G_CROP \
	VIDIOC_G_CTRL \
	VIDIOC_G_ENC_INDEX \
	VIDIOC_G_EXT_CTRLS \
	VIDIOC_G_FBUF \
	VIDIOC_G_FMT \
	VIDIOC_G_FREQUENCY \
	VIDIOC_G_INPUT \
	VIDIOC_G_JPEGCOMP \
	VIDIOC_G_MPEGCOMP \
	VIDIOC_G_MODULATOR \
	VIDIOC_G_OUTPUT \
	VIDIOC_G_PARM \
	VIDIOC_G_PRIORITY \
	VIDIOC_G_SLICED_VBI_CAP \
	VIDIOC_G_STD \
	VIDIOC_G_TUNER \
	VIDIOC_LOG_STATUS \
	VIDIOC_OVERLAY \
	VIDIOC_QBUF \
	VIDIOC_QUERYBUF \
	VIDIOC_QUERYCAP \
	VIDIOC_QUERYCTRL \
	VIDIOC_QUERYMENU \
	VIDIOC_QUERYSTD \
	VIDIOC_REQBUFS \
	VIDIOC_STREAMOFF \
	VIDIOC_STREAMON \
	VIDIOC_S_AUDIO \
	VIDIOC_S_AUDOUT \
	VIDIOC_S_CROP \
	VIDIOC_S_CTRL \
	VIDIOC_S_EXT_CTRLS \
	VIDIOC_S_FBUF \
	VIDIOC_S_FMT \
	VIDIOC_S_FREQUENCY \
	VIDIOC_S_HW_FREQ_SEEK \
	VIDIOC_S_INPUT \
	VIDIOC_S_JPEGCOMP \
	VIDIOC_S_MPEGCOMP \
	VIDIOC_S_MODULATOR \
	VIDIOC_S_OUTPUT \
	VIDIOC_S_PARM \
	VIDIOC_S_PRIORITY \
	VIDIOC_S_STD \
	VIDIOC_S_TUNER \
	VIDIOC_TRY_ENCODER_CMD \
	VIDIOC_TRY_EXT_CTRLS \
	VIDIOC_TRY_FMT \

TYPES = \
	v4l2_std_id \

ENUMS = \
	v4l2_buf_type \
	v4l2_colorspace \
	v4l2_ctrl_type \
	v4l2_exposure_auto_type \
	v4l2_field \
	v4l2_frmivaltypes \
	v4l2_frmsizetypes \
	v4l2_memory \
	v4l2_mpeg_audio_ac3_bitrate \
	v4l2_mpeg_audio_crc \
	v4l2_mpeg_audio_emphasis \
	v4l2_mpeg_audio_encoding \
	v4l2_mpeg_audio_l1_bitrate \
	v4l2_mpeg_audio_l2_bitrate \
	v4l2_mpeg_audio_l3_bitrate \
	v4l2_mpeg_audio_mode \
	v4l2_mpeg_audio_mode_extension \
	v4l2_mpeg_audio_sampling_freq \
	v4l2_mpeg_cx2341x_video_chroma_spatial_filter_type \
	v4l2_mpeg_cx2341x_video_luma_spatial_filter_type \
	v4l2_mpeg_cx2341x_video_median_filter_type \
	v4l2_mpeg_cx2341x_video_spatial_filter_mode \
	v4l2_mpeg_cx2341x_video_temporal_filter_mode \
	v4l2_mpeg_stream_type \
	v4l2_mpeg_stream_vbi_fmt \
	v4l2_mpeg_video_aspect \
	v4l2_mpeg_video_bitrate_mode \
	v4l2_mpeg_video_encoding \
	v4l2_power_line_frequency \
	v4l2_priority \
	v4l2_tuner_type \

STRUCTS = \
	v4l2_audio \
	v4l2_audioout \
	v4l2_buffer \
	v4l2_capability \
	v4l2_captureparm \
	v4l2_clip \
	v4l2_control \
	v4l2_crop \
	v4l2_cropcap \
	v4l2_dbg_chip_ident \
	v4l2_dbg_match \
	v4l2_dbg_register \
	v4l2_enc_idx \
	v4l2_enc_idx_entry \
	v4l2_encoder_cmd \
	v4l2_ext_control \
	v4l2_ext_controls \
	v4l2_fmtdesc \
	v4l2_format \
	v4l2_fract \
	v4l2_framebuffer \
	v4l2_frequency \
	v4l2_frmival_stepwise \
	v4l2_frmivalenum \
	v4l2_frmsize_discrete \
	v4l2_frmsize_stepwise \
	v4l2_frmsizeenum \
	v4l2_hw_freq_seek \
	v4l2_input \
	v4l2_jpegcompression \
	v4l2_modulator \
	v4l2_mpeg_vbi_fmt_ivtv \
	v4l2_output \
	v4l2_outputparm \
	v4l2_pix_format \
	v4l2_queryctrl \
	v4l2_querymenu \
	v4l2_rect \
	v4l2_requestbuffers \
	v4l2_sliced_vbi_cap \
	v4l2_sliced_vbi_data \
	v4l2_sliced_vbi_format \
	v4l2_standard \
	v4l2_streamparm \
	v4l2_timecode \
	v4l2_tuner \
	v4l2_vbi_format \
	v4l2_window \

ERRORS = \
	EACCES \
	EAGAIN \
	EBADF \
	EBUSY \
	EFAULT \
	EIO \
	EINTR \
	EINVAL \
	ENFILE \
	ENOMEM \
	ENOTTY \
	ENXIO \
	EMFILE \
	EPERM \
	ERANGE \

ESCAPE = \
	-e "s/&/\\&amp;/g" \
	-e "s/</\\&lt;/g" \
	-e "s/>/\\&gt;/g"

# Generate references to these structs in videodev2.h.sgml.
DOCUMENTED = \
	-e "s/\(enum *\)v4l2_mpeg_cx2341x_video_\([a-z]*_spatial_filter_type\)/\1<link linkend=\"\2\">v4l2_mpeg_cx2341x_video_\2<\/link>/g" \
	-e "s/\(\(enum\|struct\) *\)\(v4l2_[a-zA-Z0-9_]*\)/\1<link linkend=\"\3\">\3<\/link>/g" \
	-e "s/\(V4L2_PIX_FMT_[A-Z0-9_]\+\) /<link linkend=\"\1\">\1<\/link> /g" \
	-e ":a;s/\(linkend=\".*\)_\(.*\">\)/\1-\2/;ta"

capture.c.sgml: ../v4l2-apps/test/capture-example.c Makefile
	echo "<programlisting>" > $@
	expand --tabs=8 < $< | \
	  sed $(ESCAPE) $(DOCUMENTED) | \
	  sed 's/i\.e\./&ie;/' >> $@
	echo "</programlisting>" >> $@

videodev2.h.sgml: ../linux/include/linux/videodev2.h Makefile
	echo "<programlisting>" > $@
	expand --tabs=8 < $< | \
	  sed $(ESCAPE) $(DOCUMENTED) | \
	  sed 's/i\.e\./&ie;/' >> $@
	echo "</programlisting>" >> $@

checks:
	# Warn about <constant>FOO-BAR</constant> which should be
	# &FOO-BAR; or <constant>FOO_BAR</constant>.
	(! grep '>[A-Z]+-[A-Z-]+<' *.sgml)
	# Don't end a refpurpose blob with a period.
	(! grep '\.</refpurpose>' *.sgml)
	# Catch typos in error code names.
	errs=`grep '<errorcode>[^<]\+</errorcode>' *.sgml | \
	      sed 's/.*<errorcode>//;s/<\/errorcode>.*//' | sort -u` ; \
	for err in $$errs; do \
	  if ! echo $(ERRORS) | grep -q $$err ; then \
	    echo Unknown error code $$err ; exit 1 ; \
	  fi ; \
	done
	# Use the respective entities instead.
	for abbrev in "e\\.g\\." "i\\.e\\." "i2c" ; do \
	  grep "[^a-z&]$$abbrev[^a-z.;-]" *.sgml | \
	  grep -v -E "(ENTITY|videodev2\.h)" && exit 1 ; \
	done ; \
	exit 0

# We're generating these entities to avoid typos, to enforce consistent
# naming, an to be warned about missing items (Jade screams bloody murder
# if references are dangling).
entities.sgml: Makefile
	echo "<!-- Generated file! Do not edit. -->" >$@
	echo -e "\n<!-- Functions -->" >>$@
	for ident in $(FUNCS) ; do \
	  entity=`echo $$ident | tr _ -` ; \
	  echo "<!ENTITY func-$$entity \"<link" \
	    "linkend='func-$$entity'><function>$$ident()</function></link>\">" \
	  >>$@ ; \
	done
	echo -e "\n<!-- Ioctls -->" >>$@
	for ident in $(IOCTLS) ; do \
	  entity=`echo $$ident | tr _ -` ; \
	  id=`grep "<refname>$$ident" vidioc-*.sgml | sed 's/\.sgml.*//'` ; \
	  echo "<!ENTITY $$entity \"<link" \
	    "linkend='$$id'><constant>$$ident</constant></link>\">" \
	  >>$@ ; \
	done
	echo -e "\n<!-- Types -->" >>$@
	for ident in $(TYPES) ; do \
	  entity=`echo $$ident | tr _ -` ; \
	  echo "<!ENTITY $$entity \"<link" \
	    "linkend='$$entity'>$$ident</link>\">" >>$@ ; \
	done
	echo -e "\n<!-- Enums -->" >>$@
	for ident in $(ENUMS) ; do \
	  entity=`echo $$ident | sed -e "s/v4l2_mpeg_cx2341x_video_\([a-z]*_spatial_filter_type\)/\1/" | tr _ -` ; \
	  echo "<!ENTITY $$entity \"enum&nbsp;<link" \
	    "linkend='$$entity'>$$ident</link>\">" >>$@ ; \
	done
	echo -e "\n<!-- Structures -->" >>$@
	for ident in $(STRUCTS) ; do \
	  entity=`echo $$ident | tr _ -` ; \
	  echo "<!ENTITY $$entity \"struct&nbsp;<link" \
	    "linkend='$$entity'>$$ident</link>\">" >>$@ ; \
	done
	echo -e "\n<!-- Error Codes -->" >>$@
	for ident in $(ERRORS) ; do \
	  echo "<!ENTITY $$ident \"<errorcode>$$ident</errorcode>" \
	    "error code\">" >>$@ ; \
	done
	echo -e "\n<!-- Subsections -->" >>$@
	for file in $(SGMLS) ; do \
	  entity=`echo "$$file" | sed 's/.sgml//;s/\./-/g'` ; \
	  if ! echo "$$file" | \
	    grep -q -E -e '^(func|vidioc|pixfmt)-' ; then \
	    echo "<!ENTITY sub-$$entity SYSTEM \"$$file\">" >>$@ ; \
	  fi ; \
	done
	echo -e "\n<!-- Function Reference -->" >>$@
	for file in $(SGMLS) ; do \
	  if echo "$$file" | \
	    grep -q -E -e '^(func|vidioc|pixfmt)-' ; then \
	    entity=`echo "$$file" | sed 's/^[^-]*/sub/;s/.sgml//'` ; \
	    echo "<!ENTITY $$entity SYSTEM \"$$file\">" >>$@ ; \
	  fi ; \
	done

# Jade can auto-generate a list-of-tables, which includes all structs,
# but we only want data types, all types, and sorted please.
indices.sgml: Makefile
	echo "<!-- Generated file! Do not edit. -->" >$@
	echo -e "\n<index><title>List of Types</title>" >>$@
	for ident in $(TYPES) ; do \
	  id=`echo $$ident | tr _ -` ; \
	  echo "<indexentry><primaryie><link" \
	    "linkend='$$id'>$$ident</link></primaryie></indexentry>" >>$@ ; \
	done
	for ident in $(ENUMS) ; do \
	  id=`echo $$ident | sed -e "s/v4l2_mpeg_cx2341x_video_\([a-z]*_spatial_filter_type\)/\1/" | tr _ -`; \
	  echo "<indexentry><primaryie>enum&nbsp;<link" \
	    "linkend='$$id'>$$ident</link></primaryie></indexentry>" >>$@ ; \
	done
	for ident in $(STRUCTS) ; do \
	  id=`echo $$ident | tr _ -` ; \
	  echo "<indexentry><primaryie>struct&nbsp;<link" \
	    "linkend='$$id'>$$ident</link></primaryie></indexentry>" >>$@ ; \
	done
	echo "</index>" >>$@

# HTML version.
html-build.stamp: Makefile checks $(SGMLS) $(GIFPICS)
	rm -rf v4l2
	if which docbook2html >/dev/null ; then \
	  docbook2html $(WARN) $(CUSTOM_HTML) --output v4l2 v4l2.sgml ; \
	else \
	  db2html $(WARN) $(CUSTOM_HTML) --output v4l2 v4l2.sgml ; \
	fi
	cp $(GIFPICS) v4l2/
	cd v4l2 ; \
	test -e index.html || ln -s book1.htm index.html ; \
	test -e capture-example.html || \
	  ln -s `grep -l getopt_long *.htm` capture-example.html
	chmod a+rX -R v4l2
	touch html-build.stamp

# For online version. When you have a dial-up connection a single file
# is more convenient than clicking through dozens of pages.
html-single-build.stamp: Makefile checks $(SGMLS) $(GIFPICS)
	rm -rf v4l2-single
	if which docbook2html >/dev/null; then \
	  docbook2html $(WARN) $(CUSTOM_HTML) --nochunks \
	    --output v4l2-single v4l2.sgml ; \
	else \
	  db2html $(WARN) $(CUSTOM_HTML) --nochunks \
	    --output v4l2-single v4l2.sgml ; \
	fi
	cp $(GIFPICS) v4l2-single/
	chmod a+rX -R v4l2-single
	touch html-single-build.stamp

# For printing.
pdf-build.stamp: Makefile checks $(SGMLS) $(PDFPICS)
	if which docbook2pdf >/dev/null; then \
	  docbook2pdf $(WARN) $(CUSTOM_PRINT) v4l2.sgml ; \
	else \
	  db2pdf $(WARN) $(CUSTOM_PRINT) v4l2.sgml ; \
	fi
	touch pdf-build.stamp

# Example.
capture: ../v4l2-apps/test/capture-example.c
	gcc $^ -o $@

distclean clean:
	rm -f *.stamp
	rm -f videodev2.h.sgml
	rm -f capture.c.sgml
	rm -f capture
	rm -f indices.sgml entities.sgml
	rm -rf v4l2 v4l2-single v4l2.pdf
