/*
 *
 * Copyright (c) 2008  Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h" //krazy:exclude=includes
#include "setup.h"
#include "shellscript.h"
#include "testrunner.h"

#include <KAboutData>

#include <QDebug>
#include <KLocalizedString>

#include <signal.h>
#include <QApplication>
#include <QCommandLineParser>
#include <QSessionManager>

static SetupTest *setup = nullptr;
static TestRunner *runner = nullptr;

void sigHandler(int signal)
{
    qDebug() << "Received signal" << signal;
    static int sigCounter = 0;
    if (sigCounter == 0) {   // try clean shutdown
        if (runner) {
            runner->terminate();
        }
        if (setup) {
            setup->shutdown();
        }
    } else if (sigCounter == 1) {   // force shutdown
        if (setup) {
            setup->shutdownHarder();
        }
    } else { // give up and just exit
        exit(255);
    }
    ++sigCounter;
}

int main(int argc, char **argv)
{
    KAboutData aboutdata(QStringLiteral("akonadi-TES"),
                         i18n("Akonadi Testing Environment Setup"),
                         QStringLiteral("1.0"),
                         i18n("Setup Environment"),
                         KAboutLicense::GPL,
                         i18n("(c) 2008 Igor Trindade Oliveira"));

    QApplication app(argc, argv);
    app.setQuitLockEnabled(false);
    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutdata);
    parser.addOption({ { QStringLiteral("c"), QStringLiteral("config") },
                       i18n("Configuration file to open"), QStringLiteral("configfile"),
                       QStringLiteral("config.xml") });
    parser.addOption({ { QStringLiteral("b"), QStringLiteral("backend") },
                       i18n("Database backend"), QStringLiteral("backend"), QStringLiteral("sqlite") });
    parser.addOption({ QStringList{ QStringLiteral("!+[test]") },
                       i18n("Test to run automatically, interactive if none specified"), QString() });
    parser.addOption({ QStringList{ QStringLiteral("testenv") },
                       i18n("Path where testenvironment would be saved"), QStringLiteral("path") });

    aboutdata.setupCommandLine(&parser);
    parser.process(app);
    aboutdata.processCommandLine(&parser);

    auto disableSessionManagement = [](QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(qApp, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(qApp, &QGuiApplication::saveStateRequest, disableSessionManagement);

    if (parser.isSet(QStringLiteral("config"))) {
        const auto backend = parser.value(QStringLiteral("backend"));
        if (backend != QLatin1String("sqlite")
            && backend != QLatin1String("mysql")
            && backend != QLatin1String("pgsql")) {
            qCritical("Invalid backend specified. Supported values are: sqlite,mysql,pgsql");
            return 1;
        }

        Config::instance(parser.value(QStringLiteral("config")));

        if (!Config::instance()->setDbBackend(backend)) {
            qCritical("Current configuration does not support the selected backend");
            return 1;
        }
    }

#ifdef Q_OS_UNIX
    signal(SIGINT, sigHandler);
    signal(SIGQUIT, sigHandler);
#endif

    setup = new SetupTest();

    if (!setup->startAkonadiDaemon()) {
        delete setup;
        qCritical("Failed to start Akonadi server!");
        return 1;
    }

    ShellScript sh;
    sh.setEnvironmentVariables(setup->environmentVariables());

    if (parser.isSet(QStringLiteral("testenv"))) {
        sh.makeShellScript(parser.value(QStringLiteral("testenv")));
    } else {
#ifdef Q_OS_WIN
        sh.makeShellScript(setup->basePath() + QLatin1String("testenvironment.ps1"));
#else
        sh.makeShellScript(setup->basePath() + QLatin1String("testenvironment.sh"));
#endif
    }

    if (!parser.positionalArguments().isEmpty()) {
        QStringList testArgs;
        for (int i = 0; i < parser.positionalArguments().count(); ++i) {
            testArgs << parser.positionalArguments().at(i);
        }
        runner = new TestRunner(testArgs);
        QObject::connect(setup, &SetupTest::setupDone, runner, &TestRunner::run);
        QObject::connect(setup, &SetupTest::serverExited, runner, &TestRunner::triggerTermination);
        QObject::connect(runner, &TestRunner::finished, setup, &SetupTest::shutdown);
    }

    int exitCode = app.exec();
    if (runner) {
        exitCode += runner->exitCode();
        delete runner;
    }

    delete setup;
    setup = nullptr;

    return exitCode;
}
