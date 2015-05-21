/*
    Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "tagremove.h"
#include "storage/querybuilder.h"
#include "storage/selectquerybuilder.h"
#include "storage/queryhelper.h"
#include "storage/datastore.h"

#include <private/protocol_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

bool TagRemove::parseStream()
{
    Protocol::DeleteTagCommand cmd;
    mInStream >> cmd;

    if (!checkScopeConstraints(cmd.scope(), Scope::Uid)) {
        return failureResponse<Protocol::DeleteTagResponse>(
            QStringLiteral("Only UID-based TAGREMOVE is supported"));
    }

    SelectQueryBuilder<Tag> tagQuery;
    QueryHelper::setToQuery(cmd.scope(), Tag::idFullColumnName(), tagQuery);
    if (!tagQuery.exec()) {
        return failureResponse<Protocol::DeleteTagResponse>(
            QStringLiteral("Failed to obtain tags"));
    }

    const Tag::List tags = tagQuery.result();

    if (!DataStore::self()->removeTags(tags)) {
        return failureResponse<Protocol::DeleteTagResponse>(
            QStringLiteral("Failed to remove tags"));
    }

    mOutStream << Protocol::DeleteTagResponse();
    return true;
}
