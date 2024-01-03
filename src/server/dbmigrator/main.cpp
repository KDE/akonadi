/*
    SPDX-FileCopyrightText: 2024 g10 Code GmbH

    SPDX-FileContributor: Daniel Vrátil <dvratil@kde.org>
*/

#include "akonadifull-version.h"
#include "dbmigrator.h"

#include <KAboutData>
#include <KAboutLicense>
#include <KLocalizedString>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>

#include <iostream>

using namespace Akonadi::Server;

class CLIDelegate : public UIDelegate
{
public:
    Result questionYesNo(const QString &question) override
    {
        std::cout << qUtf8Printable(question)
                  << qUtf8Printable(i18nc("Options for user to input to command line prompt, parenthesis indicate "
                                          "which letter to type, capitalized option is default",
                                          " [(y)es/(N)o] "));
        std::cout.flush();
        std::string answer;
        std::getline(std::cin, answer);

        const auto yes = i18nc("Letter for option \"(y)es\" prompted from command line", "y");

        if (answer.size() > 0 && std::tolower(answer[0]) == yes[0].toLatin1()) {
            return Result::Yes;
        }
        return Result::No;
    }

    Result questionYesNoSkip(const QString &question) override
    {
        std::cout << qUtf8Printable(question)
                  << qUtf8Printable(i18nc("Options for user to input to command line prompt, parenthesis indicate "
                                          "which letter to type, capitalized option is default",
                                          " [(y)es/(N)o/(s)kip] "));

        std::cout.flush();
        std::string answer;
        std::getline(std::cin, answer);

        const auto yes = i18nc("Letter for option \"(y)es\" prompted from command line", "y");
        const auto skip = i18nc("Letter for option \"(s)kip\" prompted from command line", "s");

        if (answer.size() > 0 && std::tolower(answer[0]) == yes[0].toLatin1()) {
            return Result::Yes;
        }
        if (answer.size() > 0 && std::tolower(answer[0]) == skip[0].toLatin1()) {
            return Result::Skip;
        }
        return Result::No;
    }
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("akonadidbmigrator");
    KAboutData about(QStringLiteral("akonadi-db-migrator"),
                     i18n("Akonadi DB Migrator"),
                     QStringLiteral(AKONADI_FULL_VERSION),
                     i18n("Akonadi DB Migrator"),
                     KAboutLicense::LGPL,
                     i18n("(c) 2024 g10 Code GmbH"));
    KAboutData::setApplicationData(about);

    QCommandLineParser parser;
    about.setupCommandLine(&parser);
    const QCommandLineOption engineOpt{QStringLiteral("newengine"),
                                       QStringLiteral("The new DB engine to use. Possible values are  \"sqlite\", \"mysql\" and \"postgres\")"),
                                       QStringLiteral("ENGINE")};
    parser.addOption(engineOpt);
    parser.process(app);

    const auto targetEngine = parser.value(engineOpt).toLower();
    if (targetEngine != QLatin1String("sqlite") && targetEngine != QLatin1String("mysql") && targetEngine != QLatin1String("postgres")) {
        std::cerr << qUtf8Printable(i18nc("@info:shell", "Invalid target engine: %1.", targetEngine)) << std::endl;
        return 1;
    }

    CLIDelegate delegate;
    DbMigrator migrator(targetEngine, &delegate);
    QObject::connect(&migrator, &DbMigrator::info, &app, [](const QString &message) {
        std::cout << qUtf8Printable(message) << std::endl;
    });
    QObject::connect(&migrator, &DbMigrator::error, &app, [](const QString &message) {
        std::cerr << qUtf8Printable(message) << std::endl;
    });
    QObject::connect(&migrator, &DbMigrator::migrationCompleted, &app, [](bool success) {
        if (success) {
            std::cout << qUtf8Printable(i18nc("@info:status", "Migration completed successfully.")) << std::endl;
        } else {
            std::cout << qUtf8Printable(i18nc("@info:status", "Migration failed.")) << std::endl;
        }
        qApp->quit();
    });
    QObject::connect(&migrator, &DbMigrator::progress, &app, [](const QString &table, int tablesDone, int tablesTotal) {
        std::cout << qUtf8Printable(i18nc("@info:progress", "Migrating table %1 (%2/%3)...", table, tablesDone, tablesTotal)) << std::endl;
    });
    QString lastTable;
    int lastPerc = -1;
    QObject::connect(&migrator, &DbMigrator::tableProgress, &app, [&lastTable, &lastPerc](const QString &table, int rowsDone, int rowsTotal) {
        const int perc = rowsDone * 100 / rowsTotal;
        if (lastTable != table) {
            lastPerc = -1;
        }
        if (perc % 10 != 0 || perc == lastPerc) {
            return;
        }
        lastPerc = perc;
        lastTable = table;
        std::cout << qUtf8Printable(i18nc("@info:progress", "%1%...", perc)) << std::endl;
    });

    QMetaObject::invokeMethod(&migrator, &DbMigrator::startMigration, Qt::QueuedConnection);

    return app.exec();
}