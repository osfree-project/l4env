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

#include <qmessagebox.h>

// ***************************************************************************

void runProgram(const QString binary);
bool quitIsSafe();

// ***************************************************************************


void QRunWindow::init()
{
    connect(&statusLabelTimer, SIGNAL(timeout()), statusLabel, SLOT(clear()));
}


void QRunWindow::fileExit()
{
    
}




void QRunWindow::helpAbout()
{
    qDebug("about");
}



void QRunWindow::runCustomSlot()
{
    QString binary = runInput->text();
    
    if (binary.length() > 0)
	runProgram(binary);
    else
    {
	statusLabel->setText("No binary or loader script specified!");
	statusLabelTimer.start(5000, true);
    }
}


void QRunWindow::runProgram(const QString binary)
{    
    bool ok = loaderRun(binary);    
    
    if (ok)
	emit runSuccess(binary);
    else
	emit runFailed(binary);
}




void QRunWindow::runSuccessSlot( const QString &binary )
{
    runInput->clear();
    statusLabel->setText(QString("Started <i>") + binary + "</i>");
    statusLabelTimer.start(5000, true);
}


void QRunWindow::runFailedSlot( const QString &binary )
{
    statusLabel->setText(QString("<b>Failed to start</b> <i>") + binary + "</i>");
    statusLabelTimer.start(5000, true);
}



void QRunWindow::widgetsSlot()
{
    runProgram("qt3_widgets");
}


void QRunWindow::qvvSlot()
{
    runProgram("qt3_qvv");
}


void QRunWindow::sheepSlot()
{
    runProgram("qt3_sheep");
}


void QRunWindow::qtinyeditorSlot()
{
    runProgram("qt3_qtinyeditor");
}


void QRunWindow::topSlot()
{
    runProgram("qt3_top");
}


void QRunWindow::historySlot( const QString &binary )
{
    historyBox->insertItem(QString("Program or loader script '") + binary + "' successfully started");
}


bool QRunWindow::quitSlot()
{
    if ( !quitIsSafe() &&
	 QMessageBox::warning(this, "Do you really want to quit?",
			      "This application is managing the Qt desktop. Quitting it\n"
			      "will shutdown the whole desktop session including\n"
			      "all applications.\n"
			      "\n"	
			      "Click 'Ok' if this is what you want.",
			      QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Cancel)
	return false;
    
    return true;
}
	


void QRunWindow::closeEvent(QCloseEvent *e)
{
    if (quitSlot())
	QMainWindow::closeEvent(e);
}


