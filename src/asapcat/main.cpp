/***************************************************************************
 *   SPDX-FileCopyrightText: 2013 Volker Krause <vkrause@kde.org>          *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "session.h"

#include <shared/akapplication.h>

#include <QCoreApplication>

int main(int argc, char **argv)
{
    AkCoreApplication app(argc, argv);
    app.setDescription(QStringLiteral("Akonadi ASAP cat\n"
                                      "This is a development tool, only use this if you know what you are doing."));

    app.addPositionalCommandLineOption(QStringLiteral("input"), QStringLiteral("Input file to read commands from"));
    app.parseCommandLine();

    const QStringList args = app.commandLineArguments().positionalArguments();
    if (args.isEmpty()) {
        app.printUsage();
        return -1;
    }

    Session session(args[0]);
    QObject::connect(&session, &Session::disconnected, QCoreApplication::instance(), &QCoreApplication::quit);
    QMetaObject::invokeMethod(&session, &Session::connectToHost, Qt::QueuedConnection);

    const int result = app.exec();
    session.printStats();
    return result;
}
