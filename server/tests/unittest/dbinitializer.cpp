/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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
#include "dbinitializer.h"

#include "akdebug.h"
#include <storage/querybuilder.h>
#include <storage/datastore.h>

using namespace Akonadi;
using namespace Akonadi::Server;

DbInitializer::~DbInitializer()
{
    cleanup();
}

Resource DbInitializer::createResource(const char *name)
{
    Resource res;
    qint64 id = -1;
    res.setName(QLatin1String(name));
    bool ret = res.insert(&id);
    Q_ASSERT(ret);
    mResource = res;
    return res;
}

Collection DbInitializer::createCollection(const char *name, const Collection &parent)
{
    Collection col;
    if (parent.isValid()) {
        col.setParent(parent);
    }
    col.setName(QLatin1String(name));
    col.setRemoteId(QLatin1String(name));
    col.setResource(mResource);
    Q_ASSERT(col.insert());
    return col;
}

PimItem DbInitializer::createItem(const char *name, const Collection &parent)
{
    PimItem item;
    MimeType mimeType = MimeType::retrieveByName(QLatin1String("test"));
    if (!mimeType.isValid()) {
        MimeType mt;
        mt.setName(QLatin1String("test"));
        mt.insert();
        mimeType = mt;
    }
    item.setMimeType(mimeType);
    item.setCollection(parent);
    item.setRemoteId(QLatin1String(name));
    Q_ASSERT(item.insert());
    return item;
}

QByteArray DbInitializer::toByteArray(bool enabled)
{
    if (enabled) {
        return "TRUE";
    }
    return "FALSE";
}

QByteArray DbInitializer::toByteArray(Akonadi::Server::Tristate tristate)
{

    switch (tristate) {
        case Akonadi::Server::Tristate::True:
            return "TRUE";
        case Akonadi::Server::Tristate::False:
            return "FALSE";
        case Akonadi::Server::Tristate::Undefined:
        default:
            break;
    }
    return "DEFAULT";
}

QByteArray DbInitializer::listResponse(const Collection &col, bool ancestors, bool mimetypes, const QStringList &ancestorFetchScope)
{
    QByteArray s;
    s = "S: * " + QByteArray::number(col.id()) + " " + QByteArray::number(col.parentId()) + " (NAME \"" + col.name().toLatin1() +
        "\" MIMETYPE (";
    if (mimetypes) {
        for (int i = 0; i < col.mimeTypes().size(); i++) {
            const MimeType mt = col.mimeTypes().at(i);
            s += mt.name().toUtf8();
            if (i != (col.mimeTypes().size() - 1)) {
                s += " ";
            }
        }
    }
    s += ") REMOTEID \"" + col.remoteId().toLatin1() +
         "\" REMOTEREVISION \"\" RESOURCE \"" + col.resource().name().toLatin1() +
         "\" VIRTUAL ";
    if (col.isVirtual()) {
        s += "1";
    } else {
        s += "0";
    }
    s += " CACHEPOLICY (INHERIT true INTERVAL -1 CACHETIMEOUT -1 SYNCONDEMAND false LOCALPARTS (ALL))";
    if (ancestors) {
        s += " ANCESTORS (";
        Collection parent = col.parent();
        while (parent.isValid()) {
            s += "(" + QByteArray::number(parent.id());
            if (ancestorFetchScope.isEmpty()) {
                s += " \"" + parent.remoteId().toUtf8() + "\"";
            } else {
                s += "  (";
                if (ancestorFetchScope.contains(QLatin1String("REMOTEID"))) {
                    s += "REMOTEID \"" + parent.remoteId().toUtf8() + "\" ";
                }
                if (ancestorFetchScope.contains(QLatin1String("NAME"))) {
                    s += "NAME \"" + parent.name().toUtf8() + "\" ";
                }
                Q_FOREACH(const CollectionAttribute &attr, parent.attributes()) {
                    if (ancestorFetchScope.contains(QString::fromLatin1(attr.type()))) {
                        s += attr.type() + " \"" + attr.value() + "\" ";
                    }
                }
                s += ")";
            }
            s += ") ";
            parent = parent.parent();
        }
        s += "(0 \"\")";
        s += ")";
    }
    if (col.referenced()) {
        s += " REFERENCED TRUE";
    }
    s += " ENABLED " + toByteArray(col.enabled()) + " DISPLAY " + toByteArray(col.displayPref()) + " SYNC " + toByteArray(col.syncPref()) + " INDEX " + toByteArray(col.indexPref()) +" )";
    return s;
}

Collection DbInitializer::collection(const char *name)
{
    return Collection::retrieveByName(QLatin1String(name));
}

void DbInitializer::cleanup()
{
    Q_FOREACH(Collection col, mResource.collections()) {
        if (!col.isVirtual()) {
            col.remove();
        }
    }
    mResource.remove();

    if (DataStore::self()->database().isOpen()) {
        QueryBuilder qb( Relation::tableName(), QueryBuilder::Delete );
        qb.exec();
    }

    Q_FOREACH(PimItem item, PimItem::retrieveAll()) {
        item.remove();
    }
}

