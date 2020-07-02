/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "selftestdialog.h"

#include <KAboutData>

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCommandLineParser parser;
    KAboutData about(QStringLiteral("akonadiselftest"),
                     i18n("Akonadi Self Test"),
                     QStringLiteral("1.0"),
                     i18n("Checks and reports state of Akonadi server"),
                     KAboutLicense::GPL_V2,
                     i18n("(c) 2008 Volker Krause <vkrause@kde.org>"));
    about.setupCommandLine(&parser);

    QCoreApplication::setApplicationName(QStringLiteral("akonadiselftest"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0"));
    parser.process(app);

    Akonadi::SelfTestDialog dlg;
    dlg.show();

    return app.exec();
}
