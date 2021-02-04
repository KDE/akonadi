/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <subscriptiondialog.h>

#include <KAboutData>
#include <QApplication>
#include <QCommandLineParser>
#include <QObject>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    KAboutData aboutData(QStringLiteral("akonadi-subscriber"), QStringLiteral("Test akonadi subscriber"), QStringLiteral("0.10"));
    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    auto dlg = new Akonadi::SubscriptionDialog();
    QObject::connect(dlg, &Akonadi::SubscriptionDialog::destroyed, &app, &QApplication::quit);
    dlg->show();
    return app.exec();
}
