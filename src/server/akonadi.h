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

class Connection;
class SearchManagerThread;
class ItemRetrievalManager;
class SearchTaskManager;
class SearchManager;
class StorageJanitor;
class CacheCleaner;
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
    void serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner);
    void connectionDisconnected();

protected:
    /** reimpl */
    void incomingConnection(quintptr socketDescriptor) Q_DECL_OVERRIDE;

private:
    bool startDatabaseProcess();
    bool createDatabase();
    void stopDatabaseProcess();

protected:
    AkonadiServer(QObject *parent = Q_NULLPTR);

    CacheCleaner *mCacheCleaner;
    IntervalCheck *mIntervalCheck;
    StorageJanitor *mStorageJanitor;
    ItemRetrievalManager *mItemRetrieval;
    SearchTaskManager *mAgentSearchManager;
    QProcess *mDatabaseProcess;
    QVector<QPointer<Connection>> mConnections;
    SearchManager *mSearchManager;
    bool mAlreadyShutdown;

    static AkonadiServer *s_instance;
};

} // namespace Server
} // namespace Akonadi
#endif
