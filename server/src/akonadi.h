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
#include <QtCore/QVector>

#include <QtNetwork/QLocalServer>

class QProcess;

namespace Akonadi {
namespace Server {

class ConnectionThread;
class CacheCleaner;
class SearchManagerThread;
class ItemRetrievalThread;
class SearchTaskManagerThread;
class StorageJanitorThread;
class IntervalCheck;

class AkonadiServer : public QLocalServer
{
    Q_OBJECT

  public:
    ~AkonadiServer();
    static AkonadiServer *instance();

    /**
     * Can return a nullptr
     */
    CacheCleaner *cacheCleaner();

    /**
     * Can return a nullptr
     */
    IntervalCheck *intervalChecker();


  public Q_SLOTS:
    /**
     * Triggers a clean server shutdown.
     */
    virtual bool quit();

    virtual bool init();

  private Q_SLOTS:
    void doQuit();
    void serviceOwnerChanged ( const QString &name, const QString &oldOwner, const QString &newOwner );

  protected:
    /** reimpl */
    virtual void incomingConnection( quintptr socketDescriptor );

  private:
    void startDatabaseProcess();
    void createDatabase();
    void stopDatabaseProcess();
    void cleanOrphanCollections();

  protected:
    AkonadiServer( QObject *parent = 0 );

    CacheCleaner *mCacheCleaner;
    IntervalCheck *mIntervalChecker;
    StorageJanitorThread *mStorageJanitor;
    ItemRetrievalThread *mItemRetrievalThread;
    SearchTaskManagerThread *mAgentSearchManagerThread;
    QProcess *mDatabaseProcess;
    QVector< QPointer<ConnectionThread> > mConnections;
    SearchManagerThread *mSearchManager;
    bool mAlreadyShutdown;

    static AkonadiServer *s_instance;
};

} // namespace Server
} // namespace Akonadi
#endif
