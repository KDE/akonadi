/*
    SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "servermanager.h"

#include <QDBusConnection>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace Akonadi;

// A session bus where org.freedesktop.Akonadi.Control is activated by a script that exits with an error after 3 seconds.
class PrivateBus
{
public:
    bool start()
    {
        if (!m_dir.isValid()) {
            return false;
        }
        const QString serviceDir = m_dir.filePath(QStringLiteral("services"));
        const QString emptyBinDir = m_dir.filePath(QStringLiteral("bin"));
        const QString control = m_dir.filePath(QStringLiteral("fake_akonadi_control"));
        const QString config = m_dir.filePath(QStringLiteral("session.conf"));
        if (!QDir().mkpath(serviceDir) || !QDir().mkpath(emptyBinDir)) {
            return false;
        }
        if (!writeFile(control, "#!/bin/sh\nsleep 3\nexit 1\n") || !QFile::setPermissions(control, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner)
            || !writeFile(serviceDir + QStringLiteral("/org.freedesktop.Akonadi.Control.service"),
                          "[D-BUS Service]\nName=org.freedesktop.Akonadi.Control\nExec=" + control.toUtf8() + '\n')
            || !writeFile(config,
                          "<busconfig><type>session</type><listen>unix:tmpdir=" + m_dir.path().toUtf8() + "</listen><servicedir>" + serviceDir.toUtf8()
                              + "</servicedir><policy context=\"default\"><allow send_destination=\"*\" eavesdrop=\"true\"/>"
                                "<allow eavesdrop=\"true\"/><allow own=\"*\"/></policy></busconfig>")) {
            return false;
        }

        m_bus.start(QStringLiteral("dbus-daemon"), {QStringLiteral("--config-file=") + config, QStringLiteral("--nofork"), QStringLiteral("--print-address")});
        if (!m_bus.waitForStarted() || !m_bus.waitForReadyRead()) {
            return false;
        }
        const QByteArray address = m_bus.readLine().trimmed();
        if (address.isEmpty()) {
            return false;
        }
        qputenv("DBUS_SESSION_BUS_ADDRESS", address);
        // The QProcess fallback of ServerManager::start() finds no akonadi_control.
        qputenv("PATH", emptyBinDir.toUtf8());
        return true;
    }

    ~PrivateBus()
    {
        m_bus.kill();
        m_bus.waitForFinished();
    }

private:
    static bool writeFile(const QString &path, const QByteArray &content)
    {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(content) == content.size();
    }

    QTemporaryDir m_dir;
    QProcess m_bus;
};

class ServerManagerStartTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(QDBusConnection::sessionBus().isConnected());
    }

    void testStartDoesNotWaitForActivation()
    {
        QSignalSpy stateSpy(ServerManager::self(), &ServerManager::stateChanged);

        QElapsedTimer timer;
        timer.start();
        QVERIFY(ServerManager::start());
        QCOMPARE_LT(timer.elapsed(), 1000);
        QCOMPARE(stateSpy.count(), 1);
        QCOMPARE(stateSpy.at(0).at(0).value<ServerManager::State>(), ServerManager::Starting);

        // dbus-daemon fails the activation when the script exits, and the fallback finds no executable.
        QTRY_COMPARE_WITH_TIMEOUT(stateSpy.count(), 2, 10000);
        QCOMPARE(stateSpy.at(1).at(0).value<ServerManager::State>(), ServerManager::Broken);
    }
};

int main(int argc, char *argv[])
{
    // The session bus address is read once per process, so the bus is started before QCoreApplication.
    PrivateBus bus;
    if (!bus.start()) {
        qWarning("Could not start a private dbus-daemon");
        return 1;
    }
    QCoreApplication app(argc, argv);
    ServerManagerStartTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "servermanagerstarttest.moc"
