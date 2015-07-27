/***************************************************************************
 *   Copyright (C) 2010 by Volker Krause <vkrause@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "bridgeserver.h"
#include "bridgeconnection.h"

#include <shared/akapplication.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

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
