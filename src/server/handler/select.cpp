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

#include "connection.h"
#include "handlerhelper.h"
#include "commandcontext.h"

#include <private/scope_p.h>
#include <private/imapset_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

bool Select::parseStream()
{
    Protocol::SelectCollectionCommand cmd(m_command);

    // as per rfc, even if the following select fails, we need to reset
    connection()->context()->setCollection(Collection());

    if (!cmd.collection().uidSet().isEmpty()) {
        const Collection col = HandlerHelper::collectionFromScope(cmd.collection(), connection());
        if (!col.isValid()) {
            return failureResponse("No such collection");
        }

        connection()->context()->setCollection(col);
    }

    return successResponse<Protocol::SelectCollectionResponse>();
}
