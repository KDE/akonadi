/*
 *
 * SPDX-FileCopyrightText: 2008 Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "akonaditest_debug.h"
#include "config.h" //krazy:exclude=includes
#include "setup.h"
#include "shellscript.h"
#include "testrunner.h"

#include <KAboutData>

#include <KLocalizedString>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QSessionManager>

#include <csignal>

static std::unique_ptr<SetupTest> setup;
static std::unique_ptr<TestRunner> runner;

void sigHandler(int signal)
{
    qCCritical(AKONADITEST_LOG, "Received signal %d", signal);
    static int sigCounter = 0;
    if (sigCounter == 0) { // try clean shutdown
        if (runner) {
            runner->terminate();
        }
        if (setup) {
            setup->shutdown();
        }
    } else if (sigCounter == 1) { // force shutdown
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
    QCoreApplication app(argc, argv);
    app.setQuitLockEnabled(false);

    KAboutData aboutdata(QStringLiteral("akonadi-TES"),
                         i18n("Akonadi Testing Environment Setup"),
                         QStringLiteral("1.0"),
                         i18n("Setup Environment"),
                         KAboutLicense::GPL,
                         i18n("(c) 2008 Igor Trindade Oliveira"));
    KAboutData::setApplicationData(aboutdata);

    QCommandLineParser parser;
    parser.addOption(
        {{QStringLiteral("c"), QStringLiteral("config")}, i18n("Configuration file to open"), QStringLiteral("configfile"), QStringLiteral("config.xml")});
    parser.addOption({{QStringLiteral("b"), QStringLiteral("backend")}, i18n("Database backend"), QStringLiteral("backend"), QStringLiteral("sqlite")});
    parser.addOption({QStringList{QStringLiteral("!+[test]")}, i18n("Test to run automatically, interactive if none specified"), QString()});
    parser.addOption({QStringList{QStringLiteral("testenv")}, i18n("Path where testenvironment would be saved"), QStringLiteral("path")});

    aboutdata.setupCommandLine(&parser);
    parser.process(app);
    aboutdata.processCommandLine(&parser);

    if (parser.isSet(QStringLiteral("config"))) {
        const auto backend = parser.value(QStringLiteral("backend"));
        if (backend != QLatin1StringView("sqlite") && backend != QLatin1StringView("mysql") && backend != QLatin1StringView("pgsql")) {
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

    setup = std::make_unique<SetupTest>();

    if (!setup->startAkonadiDaemon()) {
        qCCritical(AKONADITEST_LOG, "Failed to start Akonadi server!");
        return 1;
    }

    ShellScript sh;
    sh.setEnvironmentVariables(setup->environmentVariables());

    if (parser.isSet(QStringLiteral("testenv"))) {
        sh.makeShellScript(parser.value(QStringLiteral("testenv")));
    } else {
#ifdef Q_OS_WIN
        sh.makeShellScript(setup->basePath() + QLatin1StringView("testenvironment.ps1"));
#else
        sh.makeShellScript(setup->basePath() + QLatin1StringView("testenvironment.sh"));
#endif
    }

    if (!parser.positionalArguments().isEmpty()) {
        QStringList testArgs;
        for (int i = 0; i < parser.positionalArguments().count(); ++i) {
            testArgs << parser.positionalArguments().at(i);
        }
        runner = std::make_unique<TestRunner>(testArgs);
        QObject::connect(setup.get(), &SetupTest::setupDone, runner.get(), &TestRunner::run);
        QObject::connect(setup.get(), &SetupTest::serverExited, runner.get(), &TestRunner::triggerTermination);
        QObject::connect(runner.get(), &TestRunner::finished, setup.get(), &SetupTest::shutdown);
    }

    int exitCode = app.exec();
    if (runner) {
        exitCode += runner->exitCode();
    }

    return exitCode;
}
