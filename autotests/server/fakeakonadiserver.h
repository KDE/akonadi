/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef FAKEAKONADISERVER_H
#define FAKEAKONADISERVER_H

#include "akonadi.h"
#include "exception.h"

#include <QSignalSpy>

class QLocalServer;
class QEventLoop;

Q_DECLARE_METATYPE(QList<QByteArray>)

namespace Akonadi {
namespace Server {

class FakeClient;
class FakeSearchManager;
class FakeDataStore;
class FakeConnection;

/**
 * A fake server used for testing. Losely based on KIMAP::FakeServer
 */
class FakeAkonadiServer : public AkonadiServer
{
    Q_OBJECT

public:
    static FakeAkonadiServer *instance();

    ~FakeAkonadiServer();

    /* Reimpl */
    bool init();

    /* Reimpl */
    bool quit();

    FakeDataStore *dataStore() const;

    static QString basePath();
    static QString socketFile();
    static QString instanceName();
    static QList<QByteArray> loginScenario();
    static QList<QByteArray> defaultScenario();
    static QList<QByteArray> customCapabilitiesScenario(const QList<QByteArray> &capabilities);
    static QList<QByteArray> selectCollectionScenario(const QString &name);
    static QList<QByteArray> selectResourceScenario(const QString &name);

    void setScenario(const QList<QByteArray> &scenario);

    void runTest();

    QSignalSpy *notificationSpy() const;

    void setPopulateDb(bool populate);

protected:
    /* Reimpl */
    void incomingConnection(quintptr socketDescriptor);

private:
    explicit FakeAkonadiServer();

    bool deleteDirectory(const QString &path);

    FakeDataStore *mDataStore;
    FakeSearchManager *mSearchManager;
    FakeConnection *mConnection;
    FakeClient *mClient;

    QEventLoop *mServerLoop;

    QSignalSpy *mNotificationSpy;

    bool mPopulateDb;

    static FakeAkonadiServer *sInstance;
};

AKONADI_EXCEPTION_MAKE_INSTANCE(FakeAkonadiServerException);

}
}

#endif // FAKEAKONADISERVER_H
