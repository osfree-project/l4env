/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/

#include <qfiledialog.h>

void MyMainWindow::fileNew()
{
    
}


void MyMainWindow::fileOpen()
{
    QString filename = QFileDialog::getOpenFileName();
    if ( ! filename.isNull()) {
 qDebug("Opening file '%s'", filename.latin1());
    }
    
    QFile file( filename );
    if ( !file.open( IO_ReadOnly ) )
 return;
    QTextStream ts(&file);
    textEditor->setText(ts.read());
    textEditor->setModified( FALSE );
    setCaption( filename );
}


void MyMainWindow::fileSave()
{
    
}


void MyMainWindow::fileSaveAs()
{
    
}


void MyMainWindow::filePrint()
{
    
}


void MyMainWindow::fileExit()
{
    
}


void MyMainWindow::editUndo()
{
    
}


void MyMainWindow::editRedo()
{
    
}


void MyMainWindow::editCut()
{
    
}


void MyMainWindow::editCopy()
{
    
}


void MyMainWindow::editPaste()
{
    
}


void MyMainWindow::editFind()
{
    
}


void MyMainWindow::helpIndex()
{
    
}


void MyMainWindow::helpContents()
{
    
}


void MyMainWindow::helpAbout()
{
    
}
