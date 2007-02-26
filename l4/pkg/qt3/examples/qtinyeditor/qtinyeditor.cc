/***************************************************************************
                          qtinyeditor.cpp  -  description
                             -------------------
    begin                : Tue Aug 19 20:33:02 Local time zone must be set--see zic manual page 2003
    copyright            : (C) 2003 by François Dupoux
    email                : fdupoux@free.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// Qt includes
#include <qvbox.h>
#include <qaccel.h>

// application specific includes
#include "qtinyeditorview.h"
#include "qtinyeditordoc.h"
#include "qtinyeditor.h"

#include "filenew.xpm"
#include "fileopen.xpm"
#include "filesave.xpm"

#include "config.h"

// ==================================================
QTinyEditorApp::QTinyEditorApp()
{
	QString strCaption;
	strCaption.sprintf("QTinyEditor %s", VERSION);
	
  setCaption(strCaption);

  //printer = new QPrinter;
  untitledCount=0;
  pDocList = new QList<QTinyEditorDoc>();
  pDocList->setAutoDelete(true);

  ///////////////////////////////////////////////////////////////////
  // call inits to invoke all other construction parts
  initView();
  initActions();
  initMenuBar();
  initToolBar();
  initStatusBar();
  resize( 450, 400 );

  viewToolBar->setOn(true);
  viewStatusBar->setOn(true);
}

// ==================================================
void QTinyEditorApp::closeEvent (QCloseEvent */*e*/)
{
  slotFileQuit();
}

// ==================================================
QTinyEditorApp::~QTinyEditorApp()
{
  //delete printer;
}

// ==================================================
void QTinyEditorApp::initActions()
{
  QPixmap openIcon, saveIcon, newIcon;
  newIcon = QPixmap(filenew);
  openIcon = QPixmap(fileopen);
  saveIcon = QPixmap(filesave);


  fileNew = new QAction(tr("New File"), newIcon, tr("&New"), QAccel::stringToKey(tr("Ctrl+N")), this);
  fileNew->setStatusTip(tr("Creates a new document"));
  fileNew->setWhatsThis(tr("New File\n\nCreates a new document"));
  connect(fileNew, SIGNAL(activated()), this, SLOT(slotFileNew()));

  fileOpen = new QAction(tr("Open File"), openIcon, tr("&Open..."), 0, this);
  fileOpen->setStatusTip(tr("Opens an existing document"));
  fileOpen->setWhatsThis(tr("Open File\n\nOpens an existing document"));
  connect(fileOpen, SIGNAL(activated()), this, SLOT(slotFileOpen()));

  fileSave = new QAction(tr("Save File"), saveIcon, tr("&Save"), QAccel::stringToKey(tr("Ctrl+S")), this);
  fileSave->setStatusTip(tr("Saves the actual document"));
  fileSave->setWhatsThis(tr("Save File.\n\nSaves the actual document"));
  connect(fileSave, SIGNAL(activated()), this, SLOT(slotFileSave()));

  fileSaveAs = new QAction(tr("Save File As"), tr("Save &as..."), 0, this);
  fileSaveAs->setStatusTip(tr("Saves the actual document under a new filename"));
  fileSaveAs->setWhatsThis(tr("Save As\n\nSaves the actual document under a new filename"));
  connect(fileSaveAs, SIGNAL(activated()), this, SLOT(slotFileSaveAs()));

  fileClose = new QAction(tr("Close File"), tr("&Close"), QAccel::stringToKey(tr("Ctrl+W")), this);
  fileClose->setStatusTip(tr("Closes the actual document"));
  fileClose->setWhatsThis(tr("Close File\n\nCloses the actual document"));
  connect(fileClose, SIGNAL(activated()), this, SLOT(slotFileClose()));

  /*filePrint = new QAction(tr("Print File"), tr("&Print"), QAccel::stringToKey(tr("Ctrl+P")), this);
  filePrint->setStatusTip(tr("Prints out the actual document"));
  filePrint->setWhatsThis(tr("Print File\n\nPrints out the actual document"));
  connect(filePrint, SIGNAL(activated()), this, SLOT(slotFilePrint()));*/

  fileQuit = new QAction(tr("Exit"), tr("E&xit"), QAccel::stringToKey(tr("Ctrl+Q")), this);
  fileQuit->setStatusTip(tr("Quits the application"));
  fileQuit->setWhatsThis(tr("Exit\n\nQuits the application"));
  connect(fileQuit, SIGNAL(activated()), this, SLOT(slotFileQuit()));

  editCut = new QAction(tr("Cut"), tr("Cu&t"), QAccel::stringToKey(tr("Ctrl+X")), this);
  editCut->setStatusTip(tr("Cuts the selected section and puts it to the clipboard"));
  editCut->setWhatsThis(tr("Cut\n\nCuts the selected section and puts it to the clipboard"));
  connect(editCut, SIGNAL(activated()), this, SLOT(slotEditCut()));

  editCopy = new QAction(tr("Copy"), tr("&Copy"), QAccel::stringToKey(tr("Ctrl+C")), this);
  editCopy->setStatusTip(tr("Copies the selected section to the clipboard"));
  editCopy->setWhatsThis(tr("Copy\n\nCopies the selected section to the clipboard"));
  connect(editCopy, SIGNAL(activated()), this, SLOT(slotEditCopy()));

  editUndo = new QAction(tr("Undo"), tr("&Undo"), QAccel::stringToKey(tr("Ctrl+Z")), this);
  editUndo->setStatusTip(tr("Reverts the last editing action"));
  editUndo->setWhatsThis(tr("Undo\n\nReverts the last editing action"));
  connect(editUndo, SIGNAL(activated()), this, SLOT(slotEditUndo()));

  editPaste = new QAction(tr("Paste"), tr("&Paste"), QAccel::stringToKey(tr("Ctrl+V")), this);
  editPaste->setStatusTip(tr("Pastes the clipboard contents to actual position"));
  editPaste->setWhatsThis(tr("Paste\n\nPastes the clipboard contents to actual position"));
  connect(editPaste, SIGNAL(activated()), this, SLOT(slotEditPaste()));

  viewToolBar = new QAction(tr("Toolbar"), tr("Tool&bar"), 0, this, 0, true);
  viewToolBar->setStatusTip(tr("Enables/disables the toolbar"));
  viewToolBar->setWhatsThis(tr("Toolbar\n\nEnables/disables the toolbar"));
  connect(viewToolBar, SIGNAL(toggled(bool)), this, SLOT(slotViewToolBar(bool)));

  viewStatusBar = new QAction(tr("Statusbar"), tr("&Statusbar"), 0, this, 0, true);
  viewStatusBar->setStatusTip(tr("Enables/disables the statusbar"));
  viewStatusBar->setWhatsThis(tr("Statusbar\n\nEnables/disables the statusbar"));
  connect(viewStatusBar, SIGNAL(toggled(bool)), this, SLOT(slotViewStatusBar(bool)));

  windowNewWindow = new QAction(tr("New Window"), tr("&New Window"), 0, this);
  windowNewWindow->setStatusTip(tr("Opens a new view for the current document"));
  windowNewWindow->setWhatsThis(tr("New Window\n\nOpens a new view for the current document"));
  connect(windowNewWindow, SIGNAL(activated()), this, SLOT(slotWindowNewWindow()));

  windowCascade = new QAction(tr("Cascade"), tr("&Cascade"), 0, this);
  windowCascade->setStatusTip(tr("Cascades all windows"));
  windowCascade->setWhatsThis(tr("Cascade\n\nCascades all windows"));
  connect(windowCascade, SIGNAL(activated()), pWorkspace, SLOT(cascade()));

  windowTile = new QAction(tr("Tile"), tr("&Tile"), 0, this);
  windowTile->setStatusTip(tr("Tiles all windows"));
  windowTile->setWhatsThis(tr("Tile\n\nTiles all windows"));
  connect(windowTile, SIGNAL(activated()), pWorkspace, SLOT(tile()));


  windowAction = new QActionGroup(this, 0, false);
  windowAction->insert(windowNewWindow);
  windowAction->insert(windowCascade);
  windowAction->insert(windowTile);

  helpAboutApp = new QAction(tr("About"), tr("&About..."), 0, this);
  helpAboutApp->setStatusTip(tr("About the application"));
  helpAboutApp->setWhatsThis(tr("About\n\nAbout the application"));
  connect(helpAboutApp, SIGNAL(activated()), this, SLOT(slotHelpAbout()));

}

// ==================================================
void QTinyEditorApp::initMenuBar()
{
  ///////////////////////////////////////////////////////////////////
  // MENUBAR

  ///////////////////////////////////////////////////////////////////
  // menuBar entry pFileMenu
  pFileMenu=new QPopupMenu();
  fileNew->addTo(pFileMenu);
  fileOpen->addTo(pFileMenu);
  fileClose->addTo(pFileMenu);
  pFileMenu->insertSeparator();
  fileSave->addTo(pFileMenu);
  fileSaveAs->addTo(pFileMenu);
  pFileMenu->insertSeparator();
  //filePrint->addTo(pFileMenu);
  pFileMenu->insertSeparator();
  fileQuit->addTo(pFileMenu);

  ///////////////////////////////////////////////////////////////////
  // menuBar entry editMenu
  pEditMenu=new QPopupMenu();
  editUndo->addTo(pEditMenu);
  pEditMenu->insertSeparator();
  editCut->addTo(pEditMenu);
  editCopy->addTo(pEditMenu);
  editPaste->addTo(pEditMenu);

  ///////////////////////////////////////////////////////////////////
  // menuBar entry viewMenu
  pViewMenu=new QPopupMenu();
  pViewMenu->setCheckable(true);
  viewToolBar->addTo(pViewMenu);
  viewStatusBar->addTo(pViewMenu);
  ///////////////////////////////////////////////////////////////////
  // EDIT YOUR APPLICATION SPECIFIC MENUENTRIES HERE

  ///////////////////////////////////////////////////////////////////
  // menuBar entry windowMenu
  pWindowMenu = new QPopupMenu(this);
  pWindowMenu->setCheckable(true);
  connect(pWindowMenu, SIGNAL(aboutToShow()), this, SLOT(windowMenuAboutToShow()));

  ///////////////////////////////////////////////////////////////////
  // menuBar entry helpMenu
  pHelpMenu=new QPopupMenu();
  helpAboutApp->addTo(pHelpMenu);
  pHelpMenu->insertSeparator();
  pHelpMenu->insertItem(tr("What's &This"), this, SLOT(whatsThis()), SHIFT+Key_F1);


  menuBar()->insertItem(tr("&File"), pFileMenu);
  menuBar()->insertItem(tr("&Edit"), pEditMenu);
  menuBar()->insertItem(tr("&View"), pViewMenu);
  menuBar()->insertItem(tr("&Window"), pWindowMenu);
  menuBar()->insertItem(tr("&Help"), pHelpMenu);

}

// ==================================================
void QTinyEditorApp::initToolBar()
{
  ///////////////////////////////////////////////////////////////////
  // TOOLBAR
  fileToolbar = new QToolBar(this, "file operations");
  fileNew->addTo(fileToolbar);
  fileOpen->addTo(fileToolbar);
  fileSave->addTo(fileToolbar);
  fileToolbar->addSeparator();
  QWhatsThis::whatsThisButton(fileToolbar);

}

// ==================================================
void QTinyEditorApp::initStatusBar()
{
  ///////////////////////////////////////////////////////////////////
  //STATUSBAR
  statusBar()->message(tr("Ready."));
}

// ==================================================
void QTinyEditorApp::initView()
{ 
  ////////////////////////////////////////////////////////////////////
  // set the main widget here
  QVBox* view_back = new QVBox( this );
  view_back->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
  pWorkspace = new QWorkspace( view_back );
  setCentralWidget(view_back);
}

// ==================================================
void QTinyEditorApp::createClient(QTinyEditorDoc* doc)
{
  QTinyEditorView* w = new QTinyEditorView(doc, pWorkspace,0,WDestructiveClose);
  w->installEventFilter(this);
  doc->addView(w);
  w->setText(doc->getText());
  if ( pWorkspace->windowList().isEmpty() ) // show the very first window in maximized mode
    w->showMaximized();
  else
    w->show();
}

// ==================================================
void QTinyEditorApp::openDocumentFile(const char* file)
{
  statusBar()->message(tr("Opening file..."));
  QTinyEditorDoc* doc;
	// check, if document already open. If yes, set the focus to the first view
  for(doc=pDocList->first(); doc > 0; doc=pDocList->next())
  {
    if(doc->pathName()==file)
    {
      QTinyEditorView* view=doc->firstView();	
      view->setFocus();
      return;
     }
  }
  doc = new QTinyEditorDoc();
  pDocList->append(doc);
  doc->newDocument();

  if(!file) // New file (creates an untitled)
  {
    untitledCount+=1;
    QString fileName=QString(tr("Untitled%1")).arg(untitledCount);
    doc->setPathName(fileName);
    doc->setTitle(fileName);
  }
  else // Open the file
  {
		doc->openDocument(file);
  }
  
  // create the window
  createClient(doc);

  statusBar()->message(tr("Ready."));
}

// ==================================================
bool QTinyEditorApp::queryExit(QString strFileNotSaved)
{
    QString strMsg;

    strMsg.sprintf(tr("\"%s\" was not saved. Do your really want to continue ?"), strFileNotSaved.latin1());
    int contin=QMessageBox::information(this, tr("Quit..."), strMsg, QMessageBox::Ok, QMessageBox::Cancel);

    return (contin==1);
}

// ==================================================
bool QTinyEditorApp::eventFilter(QObject* object, QEvent* event)
{
  if((event->type() == QEvent::Close)&&((QTinyEditorApp*)object!=this))
  {
    QCloseEvent* e=(QCloseEvent*)event;
    QTinyEditorView* pView=(QTinyEditorView*)object;
    QTinyEditorDoc* pDoc=pView->getDocument();
    if(pDoc->canCloseFrame(pView))
    {
      pDoc->removeView(pView);
      if(!pDoc->firstView())
        pDocList->remove(pDoc);

      e->accept();
    }
    else
      e->ignore();
  }
  return QWidget::eventFilter( object, event );    // standard event processing
}

/////////////////////////////////////////////////////////////////////
// SLOT IMPLEMENTATION
/////////////////////////////////////////////////////////////////////


// ==================================================
void QTinyEditorApp::slotFileNew()
{
  statusBar()->message(tr("Creating new file..."));

  openDocumentFile();		

  statusBar()->message(tr("Ready."));
}

// ==================================================
void QTinyEditorApp::slotFileOpen()
{
  statusBar()->message(tr("Opening file..."));

  QString fileName = QFileDialog::getOpenFileName(0,0,this);
  if (!fileName.isEmpty())
  {
     openDocumentFile(fileName);		
  }
  statusBar()->message(tr("Ready."));
}

// ==================================================
void QTinyEditorApp::slotFileSave()
{
  statusBar()->message(tr("Saving file..."));
	
  QTinyEditorView* m = (QTinyEditorView*)pWorkspace->activeWindow();
  if( m )
  {
    QTinyEditorDoc* doc = m->getDocument();
		doc->setText(m->text());
    
    if(doc->title().contains(tr("Untitled")))
      slotFileSaveAs();
    else
      if(!doc->saveDocument(doc->pathName()))
        QMessageBox::critical (this, tr("I/O Error !"), tr("Could not save the current document !"));
  }

  statusBar()->message(tr("Ready."));
}

// ==================================================
void QTinyEditorApp::slotFileSaveAs()
{
  statusBar()->message(tr("Saving file under new filename..."));

  QString fn = QFileDialog::getSaveFileName(0, 0, this);
  if (!fn.isEmpty())
  {
    QTinyEditorView* m = (QTinyEditorView*)pWorkspace->activeWindow();
    if( m )
    {
      QTinyEditorDoc* doc = m->getDocument();
		  doc->setText(m->text());
		  
      if(!doc->saveDocument(fn))
      {
         QMessageBox::critical (this, tr("I/O Error !"), tr("Could not save the current document !"));
         return;
      }
      doc->changedViewList();
    }
  }
  statusBar()->message(tr("Ready."));
}

// ==================================================
void QTinyEditorApp::slotFileClose()
{
  statusBar()->message(tr("Closing file..."));
  QTinyEditorDoc* doc;
  QTinyEditorView* m;
	
  m = (QTinyEditorView*)pWorkspace->activeWindow();
  if( m )
  {
   doc=m->getDocument();
   if(doc->isModified() && (!queryExit(doc->title())))
      return ;
   doc->closeDocument();
  }

  windowMenuAboutToShow();
  statusBar()->message(tr("Ready."));
}

// ==================================================
/*void QTinyEditorApp::slotFilePrint()
{
  statusBar()->message(tr("Printing..."));
	
  QTinyEditorView* m = (QTinyEditorView*) pWorkspace->activeWindow();
  if ( m )
    m->print( printer );

  statusBar()->message(tr("Ready."));
}*/

// ==================================================
void QTinyEditorApp::slotFileQuit()
{ 
  statusBar()->message(tr("Exiting application..."));
  ///////////////////////////////////////////////////////////////////
  // exits the Application
  QTinyEditorDoc* doc;
  //QTinyEditorView* m;
  //m = (QTinyEditorView*) pWorkspace->activeWindow();
  
  for(doc=pDocList->first(); doc > 0; doc=pDocList->next())
  {
		if (!doc->canCloseFrame(doc->firstView()))
    //if(doc->isModified() && (!queryExit(doc->title())))
      return ;
  }

  qApp->quit();

  statusBar()->message(tr("Ready."));
}

// ==================================================
void QTinyEditorApp::slotEditUndo()
{
  statusBar()->message(tr("Reverting last action..."));
	
  QTinyEditorView* m = (QTinyEditorView*) pWorkspace->activeWindow();
  if (m)
    m->undo();

  statusBar()->message(tr("Ready."));
}

// ==================================================
void QTinyEditorApp::slotEditCut()
{
  statusBar()->message(tr("Cutting selection..."));
	
  QTinyEditorView* m = (QTinyEditorView*) pWorkspace->activeWindow();
  if ( m )
    m->cut();	

  statusBar()->message(tr("Ready."));
}

// ==================================================
void QTinyEditorApp::slotEditCopy()
{
  statusBar()->message(tr("Copying selection to clipboard..."));

  QTinyEditorView* m = (QTinyEditorView*) pWorkspace->activeWindow();
  if ( m )
    m->copy();

  statusBar()->message(tr("Ready."));
}

// ==================================================
void QTinyEditorApp::slotEditPaste()
{
  statusBar()->message(tr("Inserting clipboard contents..."));
	
  QTinyEditorView* m = (QTinyEditorView*) pWorkspace->activeWindow();
  if ( m )
    m->paste();

  statusBar()->message(tr("Ready."));
}

// ==================================================
void QTinyEditorApp::slotViewToolBar(bool toggle)
{
  statusBar()->message(tr("Toggle toolbar..."));
  ///////////////////////////////////////////////////////////////////
  // turn Toolbar on or off
   if (toggle== false)
  {
    fileToolbar->hide();
  }
  else
  {
    fileToolbar->show();
  };

 statusBar()->message(tr("Ready."));
}

// ==================================================
void QTinyEditorApp::slotViewStatusBar(bool toggle)
{
  statusBar()->message(tr("Toggle statusbar..."));
  ///////////////////////////////////////////////////////////////////
  //turn Statusbar on or off
  
  if (toggle == false)
  {
    statusBar()->hide();
  }
  else
  {
    statusBar()->show();
  }

  statusBar()->message(tr("Ready."));
}

// ==================================================
void QTinyEditorApp::slotWindowNewWindow()
{
  statusBar()->message(tr("Opening new document view..."));
	
  QTinyEditorView* m = (QTinyEditorView*) pWorkspace->activeWindow();
  if ( m ){
    QTinyEditorDoc* doc = m->getDocument();
    createClient(doc);
  }

  statusBar()->message(tr("Ready."));
}

// ==================================================
void QTinyEditorApp::slotHelpAbout()
{
	QString strAbout;
	strAbout.sprintf("QTinyEditor\nVersion %s"
	                 "\n(c) 2003 by François Dupoux"
		               "\n\nWeb: http://qtinyeditor.sourceforge.net/",
		               VERSION);
  QMessageBox::about(this, tr("About..."), strAbout);
}

// ==================================================
void QTinyEditorApp::slotStatusHelpMsg(const QString &text)
{
  ///////////////////////////////////////////////////////////////////
  // change status message of whole statusbar temporary (text, msec)
  statusBar()->message(text, 2000);
}

// ==================================================
void QTinyEditorApp::windowMenuAboutToShow()
{
  pWindowMenu->clear();	
	windowNewWindow->addTo(pWindowMenu);
	windowCascade->addTo(pWindowMenu);
	windowTile->addTo(pWindowMenu);
	
  if ( pWorkspace->windowList().isEmpty() )
  {
    windowAction->setEnabled(false);
  }
  else
  {
    windowAction->setEnabled(true);
  }

  /*pWindowMenu->insertSeparator();

  QWidgetList windows = pWorkspace->windowList();
  for (int i = 0; i < int(windows.count()); ++i )
  {
    int id = pWindowMenu->insertItem(QString("&%1 ").arg(i+1)+windows.at(i)->caption(), this, SLOT( windowMenuActivated( int ) ) );
    pWindowMenu->setItemParameter( id, i );
    pWindowMenu->setItemChecked( id, pWorkspace->activeWindow() == windows.at(i) );
  }*/
}

// ==================================================
void QTinyEditorApp::windowMenuActivated( int id )
{
  QWidget* w = pWorkspace->windowList().at( id );
  if ( w )
    w->setFocus();
}

