/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "firstrun_p.h"

#include <KAboutData>
#include <QCommandLineParser>
#include <QCoreApplication>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    KAboutData aboutData(QStringLiteral("akonadi-firstrun"), QStringLiteral("Test akonadi-firstrun"), QStringLiteral("0.10"));
    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    auto f = new Akonadi::Firstrun();
    QObject::connect(f, &Akonadi::Firstrun::destroyed, &app, &QCoreApplication::quit);
    app.exec();
}
