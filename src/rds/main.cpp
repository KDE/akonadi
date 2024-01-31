/***************************************************************************
 *   SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>          *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "bridgeconnection.h"
#include "bridgeserver.h"

#include "shared/akapplication.h"

#include <QDebug>

int main(int argc, char **argv)
{
    AkCoreApplication app(argc, argv);
    app.setDescription(QStringLiteral("Akonadi Remote Debugging Server\nUse for debugging only."));
    app.parseCommandLine();
    try {
        new BridgeServer<AkonadiBridgeConnection>(31415);
        new BridgeServer<DBusBridgeConnection>(31416);
        return app.exec();
    } catch (const std::exception &e) {
        qDebug("Caught exception: %s", e.what());
        return EXIT_FAILURE;
    } catch (...) {
        qDebug("Caught unknown exception - fix the program!");
        return EXIT_FAILURE;
    }
}
