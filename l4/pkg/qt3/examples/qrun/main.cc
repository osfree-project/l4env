/* $Id$ */
/*****************************************************************************/
/**
 * \file   examples/qrun/main.cc
 * \brief  QRun startup code.
 *
 * \date   03/10/2005
 * \author Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2005-2006 Technische Universitaet Dresden
 * This file is part of the Qt3 port for L4/DROPS, which is distributed under
 * the terms of the GNU General Public License 2. Please see the COPYING file
 * for details.
 */

#include <unistd.h>
#include <stdlib.h>
#include <qapplication.h>

#include "qrunwindow.h"

// ***********************************************************

static QApplication::Type appType;

// ***********************************************************

extern bool init(const char *, const char *);

// ***********************************************************

bool quitIsSafe() {

    return (appType != QApplication::GuiServer);
}

// ***********************************************************

int main(int argc, char **argv) {

    QString binPrefix;
    QString fprov = "TFTP";
    QApplication app(argc, argv);

    appType = app.type();

    int i = 1;
    while (i + 1 < app.argc()) {
        
        if (QString(app.argv()[i]) == "--bin-prefix") {
            binPrefix = app.argv()[i + 1];
            i += 2;

        } else if (QString(app.argv()[i]) == "--fprov") {
            fprov = app.argv()[i + 1];
            i += 2;
        } else
            i++;
    }
    
    if (!init(binPrefix.latin1(), fprov.latin1()))
        exit(1);
    
    QRunWindow *w = new QRunWindow();
    app.setMainWidget(w);
    w->show();
    
    return app.exec();
}
