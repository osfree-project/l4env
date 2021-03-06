#! /bin/sh


(
    read l
    read t
    while [ ! -z "$l" ]
    do
	tt="`dirname \"$l\"`/$t"
	#echo "$l -> $t"
	[ -e "$tt" ] && ln -s "$t" "$l"
        read l
        read t
    done
) <<EOF
include/qhttp.h
../src/network/qhttp.h
include/qobjectdict.h
../src/kernel/qobjectdict.h
include/qtranslator.h
../src/kernel/qtranslator.h
include/qt.h
../src/kernel/qt.h
include/qsession.h
../src/kernel/qsession.h
include/qcache.h
../src/tools/qcache.h
include/qcheckbox.h
../src/widgets/qcheckbox.h
include/qptrdict.h
../src/tools/qptrdict.h
include/qwsproperty_qws.h
../src/kernel/qwsproperty_qws.h
include/jri_md.h
../extensions/nsplugin/src/jri_md.h
include/qhbuttongroup.h
../src/widgets/qhbuttongroup.h
include/qdropsite.h
../src/kernel/qdropsite.h
include/qurloperator.h
../src/kernel/qurloperator.h
include/qfocusdata.h
../src/kernel/qfocusdata.h
include/qmousepc_qws.h
../src/embedded/qmousepc_qws.h
include/qkeycode.h
../src/kernel/qkeycode.h
include/qkbdyopy_qws.h
../src/embedded/qkbdyopy_qws.h
include/qlist.h
../src/compat/qlist.h
include/qvariant.h
../src/kernel/qvariant.h
include/qimage.h
../src/kernel/qimage.h
include/qwidgetplugin.h
../src/widgets/qwidgetplugin.h
include/qsignal.h
../src/kernel/qsignal.h
include/qmime.h
../src/kernel/qmime.h
include/qbig5codec.h
../src/codecs/qbig5codec.h
include/qwskde2decoration_qws.h
../src/kernel/qwskde2decoration_qws.h
include/qimageformatplugin.h
../src/kernel/qimageformatplugin.h
include/qconfig-large.h
../src/tools/qconfig-large.h
include/qgfxmatroxdefs_qws.h
../src/embedded/qgfxmatroxdefs_qws.h
include/qpolygonscanner.h
../src/kernel/qpolygonscanner.h
include/qiodev.h
../src/compat/qiodev.h
include/qcolordialog.h
../src/dialogs/qcolordialog.h
include/qgfxvoodoodefs_qws.h
../src/embedded/qgfxvoodoodefs_qws.h
include/qmotifstyle.h
../src/styles/qmotifstyle.h
include/qpair.h
../src/tools/qpair.h
include/qvgroupbox.h
../src/widgets/qvgroupbox.h
include/qgfxdriverfactory_qws.h
../src/embedded/qgfxdriverfactory_qws.h
include/qmouseyopy_qws.h
../src/embedded/qmouseyopy_qws.h
include/qinterlacestyle.h
../src/styles/qinterlacestyle.h
include/qsqlform.h
../src/sql/qsqlform.h
include/qpngio.h
../src/kernel/qpngio.h
include/qdockarea.h
../src/widgets/qdockarea.h
include/qgfxdriverplugin_qws.h
../src/embedded/qgfxdriverplugin_qws.h
include/qcompactstyle.h
../src/styles/qcompactstyle.h
include/qpoint.h
../src/kernel/qpoint.h
include/qfontmetrics.h
../src/kernel/qfontmetrics.h
include/qgvector.h
../src/tools/qgvector.h
include/qobjectlist.h
../src/kernel/qobjectlist.h
include/qfontdatabase.h
../src/kernel/qfontdatabase.h
include/qrect.h
../src/kernel/qrect.h
include/qbrush.h
../src/kernel/qbrush.h
include/qwshydrodecoration_qws.h
../src/kernel/qwshydrodecoration_qws.h
include/qeuckrcodec.h
../src/codecs/qeuckrcodec.h
include/qpaintd.h
../src/compat/qpaintd.h
include/qptrlist.h
../src/tools/qptrlist.h
include/qworkspace.h
../src/workspace/qworkspace.h
include/qsize.h
../src/kernel/qsize.h
include/qgfxrepeater_qws.h
../src/embedded/qgfxrepeater_qws.h
include/qkbd_qws.h
../src/embedded/qkbd_qws.h
include/qutfcodec.h
../src/codecs/qutfcodec.h
include/qfontdialog.h
../src/dialogs/qfontdialog.h
include/jri.h
../extensions/nsplugin/src/jri.h
include/qdatastream.h
../src/tools/qdatastream.h
include/qbuttongroup.h
../src/widgets/qbuttongroup.h
include/qsocketdevice.h
../src/network/qsocketdevice.h
include/qstyleplugin.h
../src/styles/qstyleplugin.h
include/qspinbox.h
../src/widgets/qspinbox.h
include/qsortedlist.h
../src/tools/qsortedlist.h
include/qgl.h
../src/opengl/qgl.h
include/qnp.h
../extensions/nsplugin/src/qnp.h
include/qtl.h
../src/tools/qtl.h
include/qwscursor_qws.h
../src/kernel/qwscursor_qws.h
include/qrangect.h
../src/compat/qrangect.h
include/qstringlist.h
../src/tools/qstringlist.h
include/qclipboard.h
../src/kernel/qclipboard.h
include/qpushbt.h
../src/compat/qpushbt.h
include/qlcdnumber.h
../src/widgets/qlcdnumber.h
include/qvbox.h
../src/widgets/qvbox.h
include/qtstream.h
../src/compat/qtstream.h
include/qgfx_qws.h
../src/kernel/qgfx_qws.h
include/qpsprn.h
../src/compat/qpsprn.h
include/qwhatsthis.h
../src/widgets/qwhatsthis.h
include/qtextcodecfactory.h
../src/codecs/qtextcodecfactory.h
include/quuid.h
../src/tools/quuid.h
include/qconfig-dist.h
../src/tools/qconfig-dist.h
include/qtsciicodec.h
../src/codecs/qtsciicodec.h
include/qgeneric.h
../src/tools/qgeneric.h
include/qpaintdevicemetrics.h
../src/kernel/qpaintdevicemetrics.h
include/qwsmanager_qws.h
../src/kernel/qwsmanager_qws.h
include/qgfxshadowfb_qws.h
../src/embedded/qgfxshadowfb_qws.h
include/qvbuttongroup.h
../src/widgets/qvbuttongroup.h
include/qdataview.h
../src/sql/qdataview.h
include/qgrpbox.h
../src/compat/qgrpbox.h
include/qcombo.h
../src/compat/qcombo.h
include/qcolor.h
../src/kernel/qcolor.h
include/qbitarray.h
../src/tools/qbitarray.h
include/qcleanuphandler.h
../src/tools/qcleanuphandler.h
include/qcommonstyle.h
../src/styles/qcommonstyle.h
include/qmultilinedit.h
../src/compat/qmultilinedit.h
include/qkbdusb_qws.h
../src/embedded/qkbdusb_qws.h
include/qvector.h
../src/compat/qvector.h
include/qcdestyle.h
../src/styles/qcdestyle.h
include/qwsdefaultdecoration_qws.h
../src/kernel/qwsdefaultdecoration_qws.h
include/qconfig-small.h
../src/tools/qconfig-small.h
include/qpushbutton.h
../src/widgets/qpushbutton.h
include/qmouse_qws.h
../src/embedded/qmouse_qws.h
include/qeventloop.h
../src/kernel/qeventloop.h
include/qglcolormap.h
../src/opengl/qglcolormap.h
include/qgbkcodec.h
../src/codecs/qgbkcodec.h
include/qtextstream.h
../src/tools/qtextstream.h
include/jritypes.h
../extensions/nsplugin/src/jritypes.h
include/qwidgetstack.h
../src/widgets/qwidgetstack.h
include/qsemaphore.h
../src/tools/qsemaphore.h
include/qsjiscodec.h
../src/codecs/qsjiscodec.h
include/qthreadstorage.h
../src/tools/qthreadstorage.h
include/qconnection.h
../src/kernel/qconnection.h
include/qmessagebox.h
../src/dialogs/qmessagebox.h
include/qnetwork.h
../src/network/qnetwork.h
include/qwidcoll.h
../src/compat/qwidcoll.h
include/qptrqueue.h
../src/tools/qptrqueue.h
include/qsplashscreen.h
../src/widgets/qsplashscreen.h
include/qdatetimeedit.h
../src/widgets/qdatetimeedit.h
include/qchkbox.h
../src/compat/qchkbox.h
include/qfeatures.h
../src/tools/qfeatures.h
include/qcursor.h
../src/kernel/qcursor.h
include/qdesktopwidget.h
../src/kernel/qdesktopwidget.h
include/qscrbar.h
../src/compat/qscrbar.h
include/qglobal.h
../src/tools/qglobal.h
include/qrangecontrol.h
../src/widgets/qrangecontrol.h
include/qlabel.h
../src/widgets/qlabel.h
include/qwsbeosdecoration_qws.h
../src/kernel/qwsbeosdecoration_qws.h
include/qqueue.h
../src/compat/qqueue.h
include/qobjcoll.h
../src/compat/qobjcoll.h
include/qobjdefs.h
../src/compat/qobjdefs.h
include/qiodevice.h
../src/tools/qiodevice.h
include/qlistbox.h
../src/widgets/qlistbox.h
include/qsqlrecord.h
../src/sql/qsqlrecord.h
include/qvfbhdr.h
../src/kernel/qvfbhdr.h
include/qcanvas.h
../src/canvas/qcanvas.h
include/qkbddriverfactory_qws.h
../src/embedded/qkbddriverfactory_qws.h
include/qlocalfs.h
../src/kernel/qlocalfs.h
include/qwaitcondition.h
../src/tools/qwaitcondition.h
include/qassistantclient.h
../tools/assistant/lib/qassistantclient.h
include/qsizepolicy.h
../src/kernel/qsizepolicy.h
include/qgfxtransformed_qws.h
../src/embedded/qgfxtransformed_qws.h
include/qtextcodecplugin.h
../src/codecs/qtextcodecplugin.h
include/qsqlpropertymap.h
../src/sql/qsqlpropertymap.h
include/qeucjpcodec.h
../src/codecs/qeucjpcodec.h
include/qmouselinuxtp_qws.h
../src/embedded/qmouselinuxtp_qws.h
include/qprocess.h
../src/kernel/qprocess.h
include/qpixmapcache.h
../src/kernel/qpixmapcache.h
include/qdeepcopy.h
../src/tools/qdeepcopy.h
include/qsgistyle.h
../src/styles/qsgistyle.h
include/qsqlindex.h
../src/sql/qsqlindex.h
include/qprogressdialog.h
../src/dialogs/qprogressdialog.h
include/qsignalslotimp.h
../src/kernel/qsignalslotimp.h
include/qvaluelist.h
../src/tools/qvaluelist.h
include/qiconview.h
../src/iconview/qiconview.h
include/qbttngrp.h
../src/compat/qbttngrp.h
include/qwidget.h
../src/kernel/qwidget.h
include/qpicture.h
../src/kernel/qpicture.h
include/qiconset.h
../src/kernel/qiconset.h
include/qkbddriverplugin_qws.h
../src/embedded/qkbddriverplugin_qws.h
include/qgfxvnc_qws.h
../src/embedded/qgfxvnc_qws.h
include/qmemarray.h
../src/tools/qmemarray.h
include/qmultilineedit.h
../src/widgets/qmultilineedit.h
include/qasciicache.h
../src/tools/qasciicache.h
include/qsignalmapper.h
../src/kernel/qsignalmapper.h
include/qhostaddress.h
../src/network/qhostaddress.h
include/qpaintdc.h
../src/compat/qpaintdc.h
include/qpainter.h
../src/kernel/qpainter.h
include/qtabbar.h
../src/widgets/qtabbar.h
include/qobjectcleanuphandler.h
../src/kernel/qobjectcleanuphandler.h
include/qtabdlg.h
../src/compat/qtabdlg.h
include/qlineedit.h
../src/widgets/qlineedit.h
include/qprogbar.h
../src/compat/qprogbar.h
include/qprogdlg.h
../src/compat/qprogdlg.h
include/qcstring.h
../src/tools/qcstring.h
include/qkbdvr41xx_qws.h
../src/embedded/qkbdvr41xx_qws.h
include/qmetaobj.h
../src/compat/qmetaobj.h
include/qeditorfactory.h
../src/sql/qeditorfactory.h
include/qtabdialog.h
../src/dialogs/qtabdialog.h
include/qwinexport.h
../src/tools/qwinexport.h
include/qpopupmenu.h
../src/widgets/qpopupmenu.h
include/qwidgetintdict.h
../src/kernel/qwidgetintdict.h
include/qvaluestack.h
../src/tools/qvaluestack.h
include/qlined.h
../src/compat/qlined.h
include/qgroupbox.h
../src/widgets/qgroupbox.h
include/qsimplerichtext.h
../src/kernel/qsimplerichtext.h
include/qdatabrowser.h
../src/sql/qdatabrowser.h
include/qwscommand_qws.h
../src/kernel/qwscommand_qws.h
include/qwidgetlist.h
../src/kernel/qwidgetlist.h
include/qwmatrix.h
../src/kernel/qwmatrix.h
include/qmainwindow.h
../src/widgets/qmainwindow.h
include/qtextedit.h
../src/widgets/qtextedit.h
include/qsessionmanager.h
../src/kernel/qsessionmanager.h
include/qgfxvfb_qws.h
../src/embedded/qgfxvfb_qws.h
include/qgridview.h
../src/widgets/qgridview.h
include/qapplication.h
../src/kernel/qapplication.h
include/qmemorymanager_qws.h
../src/kernel/qmemorymanager_qws.h
include/qconfig-minimal.h
../src/tools/qconfig-minimal.h
include/qpixmap.h
../src/kernel/qpixmap.h
include/qtable.h
../src/table/qtable.h
include/qclipbrd.h
../src/compat/qclipbrd.h
include/qprogressbar.h
../src/widgets/qprogressbar.h
include/qaction.h
../src/widgets/qaction.h
include/qslider.h
../src/widgets/qslider.h
include/qplatinumstyle.h
../src/styles/qplatinumstyle.h
include/qscrollbar.h
../src/widgets/qscrollbar.h
include/qvaluevector.h
../src/tools/qvaluevector.h
include/qpmcache.h
../src/compat/qpmcache.h
include/qgfxmach64_qws.h
../src/embedded/qgfxmach64_qws.h
include/qmousedriverplugin_qws.h
../src/embedded/qmousedriverplugin_qws.h
include/qgb18030codec.h
../src/codecs/qgb18030codec.h
include/qwindow.h
../src/kernel/qwindow.h
include/qerrormessage.h
../src/dialogs/qerrormessage.h
include/qbuffer.h
../src/tools/qbuffer.h
include/qsocketnotifier.h
../src/kernel/qsocketnotifier.h
include/qintcach.h
../src/compat/qintcach.h
include/qsizegrip.h
../src/kernel/qsizegrip.h
include/qwindowdefs.h
../src/kernel/qwindowdefs.h
include/qtextbrowser.h
../src/widgets/qtextbrowser.h
include/qsocknot.h
../src/compat/qsocknot.h
include/qstatusbar.h
../src/widgets/qstatusbar.h
include/qsqlresult.h
../src/sql/qsqlresult.h
include/qintdict.h
../src/tools/qintdict.h
include/qsound.h
../src/kernel/qsound.h
include/qsqleditorfactory.h
../src/sql/qsqleditorfactory.h
include/qptrstack.h
../src/tools/qptrstack.h
include/qbitarry.h
../src/compat/qbitarry.h
include/qjpegio.h
../src/kernel/qjpegio.h
include/qdockwindow.h
../src/widgets/qdockwindow.h
include/qlocale.h
../src/tools/qlocale.h
include/qobject.h
../src/kernel/qobject.h
include/qptrcollection.h
../src/tools/qptrcollection.h
include/qfontinfo.h
../src/kernel/qfontinfo.h
include/qwsevent_qws.h
../src/kernel/qwsevent_qws.h
include/qevent.h
../src/kernel/qevent.h
include/qkbdsl5000_qws.h
../src/embedded/qkbdsl5000_qws.h
include/qkbdtty_qws.h
../src/embedded/qkbdtty_qws.h
include/qstack.h
../src/compat/qstack.h
include/qfiledialog.h
../src/dialogs/qfiledialog.h
include/qtoolbutton.h
../src/widgets/qtoolbutton.h
include/qpntarry.h
../src/compat/qpntarry.h
include/qtimer.h
../src/kernel/qtimer.h
include/qmousevr41xx_qws.h
../src/embedded/qmousevr41xx_qws.h
include/qwindowsystem_qws.h
../src/kernel/qwindowsystem_qws.h
include/qstyle.h
../src/kernel/qstyle.h
include/qapp.h
../src/compat/qapp.h
include/qsqldatabase.h
../src/sql/qsqldatabase.h
include/qnamespace.h
../src/kernel/qnamespace.h
include/qdir.h
../src/tools/qdir.h
include/qguardedptr.h
../src/kernel/qguardedptr.h
include/qdns.h
../src/network/qdns.h
include/qdom.h
../src/xml/qdom.h
include/qcollection.h
../src/compat/qcollection.h
include/qgif.h
../src/kernel/qgif.h
include/qftp.h
../src/network/qftp.h
include/qthread.h
../src/kernel/qthread.h
include/npapi.h
../extensions/nsplugin/src/npapi.h
include/qmap.h
../src/tools/qmap.h
include/qsqlselectcursor.h
../src/sql/qsqlselectcursor.h
include/qkeysequence.h
../src/kernel/qkeysequence.h
include/qgdict.h
../src/tools/qgdict.h
include/qpen.h
../src/kernel/qpen.h
include/qmngio.h
../src/kernel/qmngio.h
include/qsqlquery.h
../src/sql/qsqlquery.h
include/qsql.h
../src/sql/qsql.h
include/qurl.h
../src/kernel/qurl.h
include/qxml.h
../src/xml/qxml.h
include/qprinter.h
../src/kernel/qprinter.h
include/qwswindowsdecoration_qws.h
../src/kernel/qwswindowsdecoration_qws.h
include/qprndlg.h
../src/compat/qprndlg.h
include/qdirectpainter_qws.h
../src/kernel/qdirectpainter_qws.h
include/npupp.h
../extensions/nsplugin/src/npupp.h
include/qkbdpc101_qws.h
../src/embedded/qkbdpc101_qws.h
include/qprintdialog.h
../src/dialogs/qprintdialog.h
include/qsemimodal.h
../src/dialogs/qsemimodal.h
include/qcollect.h
../src/compat/qcollect.h
include/qmousedriverfactory_qws.h
../src/embedded/qmousedriverfactory_qws.h
include/qframe.h
../src/widgets/qframe.h
include/qmovie.h
../src/kernel/qmovie.h
include/qcombobox.h
../src/widgets/qcombobox.h
include/qwizard.h
../src/dialogs/qwizard.h
include/qwsutils_qws.h
../src/kernel/qwsutils_qws.h
include/qgfxsnap_qws.h
../src/embedded/qgfxsnap_qws.h
include/qwsdisplay_qws.h
../src/kernel/qwsdisplay_qws.h
include/qpointarray.h
../src/kernel/qpointarray.h
include/qasciidict.h
../src/tools/qasciidict.h
include/qconfig-medium.h
../src/tools/qconfig-medium.h
include/qtextview.h
../src/widgets/qtextview.h
include/qsoundqss_qws.h
../src/kernel/qsoundqss_qws.h
include/qfiledef.h
../src/compat/qfiledef.h
include/qfiledlg.h
../src/compat/qfiledlg.h
include/qdrawutl.h
../src/compat/qdrawutl.h
include/qmenudata.h
../src/widgets/qmenudata.h
include/qfileinf.h
../src/compat/qfileinf.h
include/qheader.h
../src/widgets/qheader.h
include/qpdevmet.h
../src/compat/qpdevmet.h
include/qgfxvga16_qws.h
../src/embedded/qgfxvga16_qws.h
include/qwidgetfactory.h
../tools/designer/uilib/qwidgetfactory.h
include/qdrawutil.h
../src/kernel/qdrawutil.h
include/qpalette.h
../src/kernel/qpalette.h
include/qcopchannel_qws.h
../src/kernel/qcopchannel_qws.h
include/qbutton.h
../src/widgets/qbutton.h
include/qwindefs.h
../src/compat/qwindefs.h
include/qradiobutton.h
../src/widgets/qradiobutton.h
include/qradiobt.h
../src/compat/qradiobt.h
include/qglist.h
../src/tools/qglist.h
include/qaccel.h
../src/kernel/qaccel.h
include/qmutex.h
../src/tools/qmutex.h
include/qfontmanager_qws.h
../src/kernel/qfontmanager_qws.h
include/qfontfactoryttf_qws.h
../src/kernel/qfontfactoryttf_qws.h
include/qpopmenu.h
../src/compat/qpopmenu.h
include/qdatetime.h
../src/tools/qdatetime.h
include/qgplugin.h
../src/kernel/qgplugin.h
include/qptrvector.h
../src/tools/qptrvector.h
include/qabstractlayout.h
../src/kernel/qabstractlayout.h
include/qgarray.h
../src/tools/qgarray.h
include/qfontinf.h
../src/compat/qfontinf.h
include/qfontfactorybdf_qws.h
../src/kernel/qfontfactorybdf_qws.h
include/qfontmet.h
../src/compat/qfontmet.h
include/qasyncimageio.h
../src/kernel/qasyncimageio.h
include/qmotifplusstyle.h
../src/styles/qmotifplusstyle.h
include/qfileinfo.h
../src/tools/qfileinfo.h
include/qmenubar.h
../src/widgets/qmenubar.h
include/qmenudta.h
../src/compat/qmenudta.h
include/qstring.h
../src/tools/qstring.h
include/qstylefactory.h
../src/styles/qstylefactory.h
include/qsqlcursor.h
../src/sql/qsqlcursor.h
include/qstrvec.h
../src/tools/qstrvec.h
include/qmsgbox.h
../src/compat/qmsgbox.h
include/qmousebus_qws.h
../src/embedded/qmousebus_qws.h
include/qgcache.h
../src/tools/qgcache.h
include/qlibrary.h
../src/tools/qlibrary.h
include/qscrollview.h
../src/widgets/qscrollview.h
include/qmlined.h
../src/compat/qmlined.h
include/qstrlist.h
../src/tools/qstrlist.h
include/qshared.h
../src/tools/qshared.h
include/qsqldriverplugin.h
../src/sql/qsqldriverplugin.h
include/qwsdecoration_qws.h
../src/kernel/qwsdecoration_qws.h
include/qjiscodec.h
../src/codecs/qjiscodec.h
include/qgfxmatrox_qws.h
../src/embedded/qgfxmatrox_qws.h
include/qintcache.h
../src/tools/qintcache.h
include/qkeyboard_qws.h
../src/kernel/qkeyboard_qws.h
include/qvalidator.h
../src/widgets/qvalidator.h
include/qlayout.h
../src/kernel/qlayout.h
include/qsqldriver.h
../src/sql/qsqldriver.h
include/qurlinfo.h
../src/kernel/qurlinfo.h
include/qtoolbar.h
../src/widgets/qtoolbar.h
include/qtoolbox.h
../src/widgets/qtoolbox.h
include/qlcdnum.h
../src/compat/qlcdnum.h
include/qregexp.h
../src/tools/qregexp.h
include/qsocket.h
../src/network/qsocket.h
include/qsqlerror.h
../src/sql/qsqlerror.h
include/qregion.h
../src/kernel/qregion.h
include/qsqlfield.h
../src/sql/qsqlfield.h
include/qgfxmach64defs_qws.h
../src/embedded/qgfxmach64defs_qws.h
include/qtooltip.h
../src/widgets/qtooltip.h
include/qdatatable.h
../src/sql/qdatatable.h
include/qjpunicode.h
../src/codecs/qjpunicode.h
include/qsettings.h
../src/tools/qsettings.h
include/qserversocket.h
../src/network/qserversocket.h
include/qnetworkprotocol.h
../src/kernel/qnetworkprotocol.h
include/qgfxraster_qws.h
../src/kernel/qgfxraster_qws.h
include/qhgroupbox.h
../src/widgets/qhgroupbox.h
include/qtabwidget.h
../src/widgets/qtabwidget.h
include/qwsregionmanager_qws.h
../src/kernel/qwsregionmanager_qws.h
include/qmetaobject.h
../src/kernel/qmetaobject.h
include/qwskdedecoration_qws.h
../src/kernel/qwskdedecoration_qws.h
include/qsplitter.h
../src/widgets/qsplitter.h
include/qlistview.h
../src/widgets/qlistview.h
include/qdialog.h
../src/dialogs/qdialog.h
include/qstylesheet.h
../src/kernel/qstylesheet.h
include/qsyntaxhighlighter.h
../src/widgets/qsyntaxhighlighter.h
include/qpaintdevicedefs.h
../src/kernel/qpaintdevicedefs.h
include/qdragobject.h
../src/kernel/qdragobject.h
include/q1xcompatibility.h
../src/kernel/q1xcompatibility.h
include/qdial.h
../src/widgets/qdial.h
include/qdict.h
../src/tools/qdict.h
include/qgfxlinuxfb_qws.h
../src/embedded/qgfxlinuxfb_qws.h
include/qgfxvoodoo_qws.h
../src/embedded/qgfxvoodoo_qws.h
include/qasyncio.h
../src/kernel/qasyncio.h
include/qfile.h
../src/tools/qfile.h
include/qfont.h
../src/kernel/qfont.h
include/qarray.h
../src/compat/qarray.h
include/qtextcodec.h
../src/codecs/qtextcodec.h
include/qbitmap.h
../src/kernel/qbitmap.h
include/qaccessible.h
../src/kernel/qaccessible.h
include/private/qeffects_p.h
../../src/widgets/qeffects_p.h
include/private/qstyleinterface_p.h
../../src/styles/qstyleinterface_p.h
include/private/qtextcodecinterface_p.h
../../src/codecs/qtextcodecinterface_p.h
include/private/qsettings_p.h
../../src/tools/qsettings_p.h
include/private/qpainter_p.h
../../src/kernel/qpainter_p.h
include/private/qfiledefs_p.h
../../src/tools/qfiledefs_p.h
include/private/qsqlextension_p.h
../../src/sql/qsqlextension_p.h
include/private/qmutex_p.h
../../src/tools/qmutex_p.h
include/private/qsvgdevice_p.h
../../src/xml/qsvgdevice_p.h
include/private/qscriptengine_p.h
../../src/kernel/qscriptengine_p.h
include/private/qpsprinter_p.h
../../src/kernel/qpsprinter_p.h
include/private/qucomextra_p.h
../../src/kernel/qucomextra_p.h
include/private/qlibrary_p.h
../../src/tools/qlibrary_p.h
include/private/qfontengine_p.h
../../src/kernel/qfontengine_p.h
include/private/qcom_p.h
../../src/tools/qcom_p.h
include/private/qfontdata_p.h
../../src/kernel/qfontdata_p.h
include/private/qkbddriverinterface_p.h
../../src/embedded/qkbddriverinterface_p.h
include/private/qrichtext_p.h
../../src/kernel/qrichtext_p.h
include/private/qtitlebar_p.h
../../src/widgets/qtitlebar_p.h
include/private/qdir_p.h
../../src/tools/qdir_p.h
include/private/qeventloop_p.h
../../src/kernel/qeventloop_p.h
include/private/qinputcontext_p.h
../../src/kernel/qinputcontext_p.h
include/private/qprinter_p.h
../../src/kernel/qprinter_p.h
include/private/qgpluginmanager_p.h
../../src/tools/qgpluginmanager_p.h
include/private/qunicodetables_p.h
../../src/tools/qunicodetables_p.h
include/private/qwidgetinterface_p.h
../../src/widgets/qwidgetinterface_p.h
include/private/qlayoutengine_p.h
../../src/kernel/qlayoutengine_p.h
include/private/qtextengine_p.h
../../src/kernel/qtextengine_p.h
include/private/qdialogbuttons_p.h
../../src/widgets/qdialogbuttons_p.h
include/private/qucom_p.h
../../src/tools/qucom_p.h
include/private/qwidget_p.h
../../src/kernel/qwidget_p.h
include/private/qmousedriverinterface_p.h
../../src/embedded/qmousedriverinterface_p.h
include/private/qlock_p.h
../../src/kernel/qlock_p.h
include/private/qtextlayout_p.h
../../src/kernel/qtextlayout_p.h
include/private/qcriticalsection_p.h
../../src/tools/qcriticalsection_p.h
include/private/qt_x11_p.h
../../src/kernel/qt_x11_p.h
include/private/qimageformatinterface_p.h
../../src/kernel/qimageformatinterface_p.h
include/private/qmutexpool_p.h
../../src/tools/qmutexpool_p.h
include/private/qsyntaxhighlighter_p.h
../../src/widgets/qsyntaxhighlighter_p.h
include/private/qcolor_p.h
../../src/kernel/qcolor_p.h
include/private/qcomponentfactory_p.h
../../src/tools/qcomponentfactory_p.h
include/private/qsharedmemory_p.h
../../src/kernel/qsharedmemory_p.h
include/private/qlocale_p.h
../../src/tools/qlocale_p.h
include/private/qisciicodec_p.h
../../src/codecs/qisciicodec_p.h
include/private/qfontcodecs_p.h
../../src/codecs/qfontcodecs_p.h
include/private/qpluginmanager_p.h
../../src/tools/qpluginmanager_p.h
include/private/qsqlmanager_p.h
../../src/sql/qsqlmanager_p.h
include/private/qgl_x11_p.h
../../src/opengl/qgl_x11_p.h
include/private/qwidgetresizehandler_p.h
../../src/widgets/qwidgetresizehandler_p.h
include/private/qthreadinstance_p.h
../../src/tools/qthreadinstance_p.h
include/private/qcomlibrary_p.h
../../src/tools/qcomlibrary_p.h
include/private/qinternal_p.h
../../src/kernel/qinternal_p.h
include/private/qapplication_p.h
../../src/kernel/qapplication_p.h
include/private/qsqldriverinterface_p.h
../../src/sql/qsqldriverinterface_p.h
include/private/qgfxdriverinterface_p.h
../../src/embedded/qgfxdriverinterface_p.h
include/qrtlcodec.h
../src/codecs/qrtlcodec.h
include/qinputdialog.h
../src/dialogs/qinputdialog.h
include/qhbox.h
../src/widgets/qhbox.h
include/qdstream.h
../src/compat/qdstream.h
include/qgrid.h
../src/widgets/qgrid.h
include/qpaintdevice.h
../src/kernel/qpaintdevice.h
include/qwindowsstyle.h
../src/styles/qwindowsstyle.h
include/qconnect.h
../src/compat/qconnect.h
include/qwssocket_qws.h
../src/kernel/qwssocket_qws.h
include/qdatetm.h
../src/compat/qdatetm.h
include/qobjectdefs.h
../src/kernel/qobjectdefs.h
EOF

exit 0
