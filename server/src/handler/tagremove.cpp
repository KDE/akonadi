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

using namespace Akonadi;
using namespace Akonadi::Server;

TagRemove::TagRemove(Scope::SelectionScope scope)
    : Handler()
    , mScope(scope)
{
}

TagRemove::~TagRemove()
{
}

bool TagRemove::parseStream()
{
    if (mScope.scope() != Scope::Uid) {
        throw HandlerException("Only UID-based TAGREMOVE is supported");
    }

    mScope.parseScope(m_streamParser);

    // Get all PIM items that we will untag
    SelectQueryBuilder<PimItem> itemsQuery;
    itemsQuery.addJoin(QueryBuilder::LeftJoin, PimItemTagRelation::tableName(), PimItemTagRelation::leftFullColumnName(), PimItem::idFullColumnName());
    QueryHelper::setToQuery(mScope.uidSet(), PimItemTagRelation::rightColumn(), itemsQuery);

    if (!itemsQuery.exec()) {
        throw HandlerException("Untagging failed");
    }
    const PimItem::List items = itemsQuery.result();

    SelectQueryBuilder<Tag> tagQuery;
    QueryHelper::setToQuery(mScope.uidSet(), Tag::idFullColumnName(), tagQuery);
    if (!tagQuery.exec()) {
        throw HandlerException("Failed to obtain tags");
    }
    const Tag::List tags = tagQuery.result();

    QSet<qint64> removedTags;
    Q_FOREACH (const Tag &tag, tags) {
        removedTags << tag.id();
    }
    if (!items.isEmpty()) {
        DataStore::self()->notificationCollector()->itemsTagsChanged(items, QSet<qint64>(), removedTags);
    }

    Q_FOREACH (const Tag &tag, tags) {
        DataStore::self()->notificationCollector()->tagRemoved(tag);
    }

    // Just remove the tags, table constraints will take care of the rest
    QueryBuilder qb(Tag::tableName(), QueryBuilder::Delete);
    QueryHelper::setToQuery(mScope.uidSet(), Tag::idFullColumnName(), qb);
    if (!qb.exec()) {
        throw HandlerException("Deletion failed");
    }

    return successResponse("TAGREMOVE complete");
}
