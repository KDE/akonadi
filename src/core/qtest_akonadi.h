/* This file is based on qtest_kde.h from kdelibs
    Copyright (C) 2006 David Faure <faure@kde.org>
    Copyright (C) 2009 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef QTEST_AKONADI_H
#define QTEST_AKONADI_H

#include "agentinstance.h"
#include "agentmanager.h"
#include "servermanager.h"
#include "collectionpathresolver.h"
#include "monitor.h"
#include "collectionfetchscope.h"
#include "itemfetchscope.h"

#include <QTest>
#include <QSignalSpy>
#include <QTimer>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

/**
* \short Akonadi Replacement for QTEST_MAIN from QTestLib
*
* This macro should be used for classes that run inside the Akonadi Testrunner.
* So instead of writing QTEST_MAIN( TestClass ) you write
* QTEST_AKONADIMAIN( TestClass ).
*
* Unlike QTEST_MAIN, this macro actually does call QApplication::exec() so
* that the application is running during test execution. This is needed
* for proper clean up of Sessions.
*
* \param TestObject The class you use for testing.
*
* \see QTestLib
* \see QTEST_KDEMAIN
*/
#define QTEST_AKONADIMAIN(TestObject) \
    int main(int argc, char *argv[]) \
    { \
        qputenv("LC_ALL", "C"); \
        qunsetenv("KDE_COLOR_DEBUG"); \
        QApplication app(argc, argv); \
        app.setApplicationName(QStringLiteral("qttest")); \
        app.setOrganizationDomain(QStringLiteral("kde.org")); \
        app.setOrganizationName(QStringLiteral("KDE")); \
        QGuiApplication::setQuitOnLastWindowClosed(false); \
        qRegisterMetaType<QList<QUrl>>(); \
        int result = 0; \
        QTimer::singleShot(0, &app, [argc, argv, &result]() { \
            TestObject tc; \
            result = QTest::qExec(&tc, argc, argv); \
            qApp->quit(); \
        }); \
        app.exec(); \
        return result; \
    }

namespace AkonadiTest
{
/**
 * Checks that the test is running in the proper test environment
 */
void checkTestIsIsolated()
{
    if (qEnvironmentVariableIsEmpty("TESTRUNNER_DB_ENVIRONMENT"))
        qFatal("This test must be run using ctest, in order to use the testrunner environment. Aborting, to avoid messing up your real akonadi");
    if (!qgetenv("XDG_DATA_HOME").contains("testrunner"))
        qFatal("Did you forget to run the test using QTEST_AKONADIMAIN?");
}

/**
 * Switch all resources offline to reduce interference from them
 */
void setAllResourcesOffline()
{
    // switch all resources offline to reduce interference from them
    const auto lst = Akonadi::AgentManager::self()->instances();
    for (Akonadi::AgentInstance agent : lst) {
        agent.setIsOnline(false);
    }
}

template<typename Object, typename Func>
bool akWaitForSignal(Object sender, Func member, int timeout = 1000)
{
    QSignalSpy spy(sender, member);
    bool ok = false;
    [&]() {
        QTRY_VERIFY_WITH_TIMEOUT(spy.count() > 0, timeout);
        ok = true;
    }();
    return ok;
}

bool akWaitForSignal(const QObject *sender, const char *member, int timeout = 1000)
{
    QSignalSpy spy(sender, member);
    bool ok = false;
    [&]() {
        QTRY_VERIFY_WITH_TIMEOUT(spy.count() > 0, timeout);
        ok = true;
    }();
    return ok;
}

qint64 collectionIdFromPath(const QString &path)
{
    Akonadi::CollectionPathResolver *resolver = new Akonadi::CollectionPathResolver(path);
    bool success = resolver->exec();
    if (!success) {
        qDebug() << "path resolution for " << path << " failed: " << resolver->errorText();
        return -1;
    }
    qint64 id = resolver->collection();
    return id;
}

QString testrunnerServiceName()
{
    const QString pid = QString::fromLocal8Bit(qgetenv("AKONADI_TESTRUNNER_PID"));
    Q_ASSERT(!pid.isEmpty());
    return QStringLiteral("org.kde.Akonadi.Testrunner-") + pid;
}

bool restartAkonadiServer()
{
    QDBusInterface testrunnerIface(testrunnerServiceName(),
                                   QStringLiteral("/"),
                                   QStringLiteral("org.kde.Akonadi.Testrunner"),
                                   QDBusConnection::sessionBus());
    if (!testrunnerIface.isValid()) {
        qWarning() << "Unable to get a dbus interface to the testrunner!";
    }

    QDBusReply<void> reply = testrunnerIface.call(QStringLiteral("restartAkonadiServer"));
    if (!reply.isValid()) {
        qWarning() << reply.error();
        return false;
    } else if (Akonadi::ServerManager::isRunning()) {
        return true;
    } else {
        bool ok = false;
        [&]() {
            QSignalSpy spy(Akonadi::ServerManager::self(), &Akonadi::ServerManager::started);
            QTRY_VERIFY_WITH_TIMEOUT(spy.count() > 0, 10000);
            ok = true;
        }();
        return ok;
    }
}

bool trackAkonadiProcess(bool track)
{
    QDBusInterface testrunnerIface(testrunnerServiceName(),
                                   QStringLiteral("/"),
                                   QStringLiteral("org.kde.Akonadi.Testrunner"),
                                   QDBusConnection::sessionBus());
    if (!testrunnerIface.isValid()) {
        qWarning() << "Unable to get a dbus interface to the testrunner!";
    }

    QDBusReply<void> reply = testrunnerIface.call(QStringLiteral("trackAkonadiProcess"), track);
    if (!reply.isValid()) {
        qWarning() << reply.error();
        return false;
    } else {
        return true;
    }
}

std::unique_ptr<Akonadi::Monitor> getTestMonitor()
{
    auto m = new Akonadi::Monitor();
    m->fetchCollection(true);
    m->setCollectionMonitored(Akonadi::Collection::root(), true);
    m->setAllMonitored(true);
    auto &itemFS = m->itemFetchScope();
    itemFS.setAncestorRetrieval(Akonadi::ItemFetchScope::All);
    auto &colFS = m->collectionFetchScope();
    colFS.setAncestorRetrieval(Akonadi::CollectionFetchScope::All);

    QSignalSpy readySpy(m, &Akonadi::Monitor::monitorReady);
    readySpy.wait();

    return std::unique_ptr<Akonadi::Monitor>(m);
}


} // namespace

/**
 * Runs an Akonadi::Job synchronously and aborts if the job failed.
 * Similar to QVERIFY( job->exec() ) but includes the job error message
 * in the output in case of a failure.
 */
#define AKVERIFYEXEC( job ) \
    QVERIFY2( job->exec(), job->errorString().toUtf8().constData() )

#endif
