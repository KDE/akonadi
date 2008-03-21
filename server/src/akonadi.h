/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/


#ifndef AKONADISERVER_H
#define AKONADISERVER_H

#include <QtCore/QPointer>

#ifdef Q_OS_WIN
#include <QtNetwork/QTcpServer>
#else
#include <klocalsocket.h>
#endif

#include "akonadiprivate_export.h"

class QProcess;
class IntervalCheck;

namespace Akonadi {

class AkonadiConnection;
class CacheCleaner;
class XesamManager;

#ifdef Q_OS_WIN
class AKONADIPRIVATE_EXPORT AkonadiServer: public QTcpServer
#else
class AKONADIPRIVATE_EXPORT AkonadiServer: public KLocalSocketServer
#endif
{
    Q_OBJECT

  public:
    static AkonadiServer* instance();

    AkonadiServer( QObject *parent = 0 );
    ~AkonadiServer();

  public Q_SLOTS:
    /**
     * Triggers a clean server shutdown.
     */
    void quit();

  private Q_SLOTS:
    void doQuit();

  protected:
    /** reimpl */
    void incomingConnection(int socketDescriptor);

  private:
    void startDatabaseProcess();
    void stopDatabaseProcess();

    CacheCleaner *mCacheCleaner;
    IntervalCheck *mIntervalChecker;
    QProcess *mDatabaseProcess;
    QList< QPointer<AkonadiConnection> > mConnections;
    XesamManager *mXesamManager;
};

}
#endif
