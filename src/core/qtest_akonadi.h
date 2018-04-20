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

#include <agentinstance.h>
#include <agentmanager.h>

#include <QTest>
#include <QSignalSpy>
#include <QTimer>

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
        app.setApplicationName(QLatin1String("qttest")); \
        app.setOrganizationDomain(QLatin1String("kde.org")); \
        app.setOrganizationName(QLatin1String("KDE")); \
        QGuiApplication::setQuitOnLastWindowClosed(false); \
        qRegisterMetaType<QList<QUrl>>(); \
        int result = 0; \
        QTimer::singleShot(0, [argc, argv, &result]() { \
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
    Q_FOREACH (Akonadi::AgentInstance agent, Akonadi::AgentManager::self()->instances()) {    //krazy:exclude=foreach
        agent.setIsOnline(false);
    }
}

bool akWaitForSignal(QObject *sender, const char *member, int timeout = 1000)
{
    QSignalSpy spy(sender, member);
    bool ok = false;
    [&]() {
        QTRY_VERIFY_WITH_TIMEOUT(spy.count() > 0, timeout);
        ok = true;
    }();
    return ok;
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
