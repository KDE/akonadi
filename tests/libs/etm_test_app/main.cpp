/*
    SPDX-FileCopyrightText: 2010 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KAboutData>
#include <KLocalizedString>
#include <QApplication>
#include <QCommandLineParser>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KAboutData aboutData(QStringLiteral("etm_test_app"),
                         i18n("ETM Test application"),
                         QStringLiteral("0.99"),
                         i18n("Test app for EntityTreeModel"),
                         KAboutLicense::GPL,
                         QStringLiteral("https://community.kde.org/KDE_PIM/Akonadi/"));
    aboutData.addAuthor(i18nc("@info:credit", "Stephen Kelly"), i18n("Author"), QStringLiteral("steveire@gmail.com"));
    KAboutData::setApplicationData(aboutData);

    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("akonadi")));
    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    MainWindow mw;
    mw.show();

    return app.exec();
}
