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

#include <QObject>
#include <QVector>

class QProcess;

namespace Akonadi
{
namespace Server
{

class Connection;
class ItemRetrievalManager;
class SearchTaskManager;
class SearchManager;
class StorageJanitor;
class CacheCleaner;
class IntervalCheck;
class AkLocalServer;
class NotificationManager;

class AkonadiServer : public QObject
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

    /**
     * Instance-aware server .config directory
     */
    QString serverPath() const;

    /**
     * Can return a nullptr
     */
    NotificationManager *notificationManager();

public Q_SLOTS:
    /**
     * Triggers a clean server shutdown.
     */
    virtual bool quit();

    virtual bool init();

protected Q_SLOTS:
    virtual void newCmdConnection(quintptr socketDescriptor);

private Q_SLOTS:
    void doQuit();
    void serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner);
    void connectionDisconnected();

private:
    bool startDatabaseProcess();
    bool createDatabase();
    void stopDatabaseProcess();
    uint userId() const;

protected:
    AkonadiServer(QObject *parent = nullptr);

    AkLocalServer *mCmdServer = nullptr;
    AkLocalServer *mNtfServer = nullptr;

    NotificationManager *mNotificationManager = nullptr;
    CacheCleaner *mCacheCleaner = nullptr;
    IntervalCheck *mIntervalCheck = nullptr;
    StorageJanitor *mStorageJanitor = nullptr;
    ItemRetrievalManager *mItemRetrieval = nullptr;
    SearchTaskManager *mAgentSearchManager = nullptr;
    QProcess *mDatabaseProcess = nullptr;
    QVector<Connection *> mConnections;
    SearchManager *mSearchManager = nullptr;
    bool mAlreadyShutdown = false;

    static AkonadiServer *s_instance;
};

} // namespace Server
} // namespace Akonadi
#endif
