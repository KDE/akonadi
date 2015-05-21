/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/
#include "select.h"

#include <QtCore/QDebug>

#include "akonadi.h"
#include "connection.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "handlerhelper.h"
#include "imapstreamparser.h"
#include "storage/selectquerybuilder.h"
#include "storage/collectionstatistics.h"
#include "commandcontext.h"

#include "response.h"

#include <private/protocol_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

bool Select::parseStream()
{
    Protocol::SelectCollectionCommand cmd;
    mInStream >> cmd;

    // as per rfc, even if the following select fails, we need to reset
    connection()->context()->setCollection(Collection());

    const Collection col = HandlerHelper::collectionFromScope(cmd.collection());
    if (!col.isValid()) {
        return failureResponse<Protocol::SelectCollectionResponse>(
            QStringLiteral("No such collection");
    }

    connection()->context()->setCollection(col);

    mOutStream << Protocol::SelectCollectionResponse();
    return true;
}
