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

void runProgram(const QString binary);

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


void QRunWindow::ftpclientSlot()
{
    runProgram("qttest_ftpclient");
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



void QRunWindow::qvvSlot()
{
    runProgram("qttest_qvv");
}


void QRunWindow::sheepSlot()
{
    runProgram("qttest_sheep");
}


void QRunWindow::qtinyeditorSlot()
{
    runProgram("qttest_qtinyeditor");
}


void QRunWindow::historySlot( const QString &binary )
{
    historyBox->insertItem(QString("Program or loader script '") + binary + "' successfully started");
}
