/***************************************************************************
 *   Copyright (C) 2006 by Ingo Kloecker <kloecker@kde.org>                *
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
#include "create.h"

#include <QtCore/QDebug>
#include <QtCore/QStringList>

#include "connection.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/transaction.h"
#include "handlerhelper.h"
#include "storage/selectquerybuilder.h"
#include "libs/protocol_p.h"

#include "response.h"
#include "libs/imapparser_p.h"
#include "imapstreamparser.h"

using namespace Akonadi;
using namespace Akonadi::Server;

Create::Create(Scope::SelectionScope scope)
    : Handler()
    , m_scope(scope)
{
}

static Tristate getTristateValue(const QByteArray &value)
{
    if (value == "TRUE") {
        return Tristate::True;
    } else if (value == "FALSE") {
        return Tristate::False;
    }
    return Tristate::Undefined;
}

bool Create::parseStream()
{
    QString name = m_streamParser->readUtf8String();
    if (name.isEmpty()) {
        return failureResponse("Invalid collection name");
    }

    bool ok = false;
    Collection parent;

    if (m_scope == Scope::Uid || m_scope == Scope::None) {
        qint64 parentId = m_streamParser->readNumber(&ok);
        if (!ok) {   // RFC 3501 compat
            QString parentPath;
            int index = name.lastIndexOf(QLatin1Char('/'));
            if (index > 0) {
                parentPath = name.left(index);
                name = name.mid(index + 1);
                parent = HandlerHelper::collectionFromIdOrName(parentPath.toUtf8());
            } else {
                parentId = 0;
            }
        } else {
            if (parentId > 0) {
                parent = Collection::retrieveById(parentId);
            }
        }

        if (parentId != 0 && !parent.isValid()) {
            return failureResponse("Parent collection not found");
        }
    } else if (m_scope == Scope::Rid) {
        const QString rid = m_streamParser->readUtf8String();
        if (rid.isEmpty()) {
            throw HandlerException("Empty parent remote identifier");
        }
        if (!connection()->context()->resource().isValid()) {
            throw HandlerException("Invalid resource context");
        }
        SelectQueryBuilder<Collection> qb;
        qb.addValueCondition(Collection::remoteIdColumn(), Query::Equals, rid);
        qb.addValueCondition(Collection::resourceIdColumn(), Query::Equals, connection()->context()->resource().id());
        if (!qb.exec()) {
            throw HandlerException("Unable to execute collection query");
        }
        const Collection::List cols = qb.result();
        if (cols.size() == 0) {
            throw HandlerException("Parent collection not found");
        } else if (cols.size() > 1) {
            throw HandlerException("Parent collection is not unique");
        }
        parent = cols.first();
    }

    qint64 resourceId = 0;
    bool forceVirtual = false;
    MimeType::List parentContentTypes;
    if (parent.isValid()) {
        // check if parent can contain a sub-folder
        parentContentTypes = parent.mimeTypes();
        bool found = false, foundVirtual = false;
        Q_FOREACH (const MimeType &mt, parentContentTypes) {
            if (mt.name() == QLatin1String("inode/directory")) {
                found = true;
                if (foundVirtual) {
                    break;
                }
            } else if (mt.name() == QLatin1String("application/x-vnd.akonadi.collection.virtual")) {
                foundVirtual = true;
                if (found) {
                    break;
                }
            }
        }
        if (!found && !foundVirtual) {
            return failureResponse("Parent collection can not contain sub-collections");
        }

        // If only virtual collections are supported, force every new collection to
        // be virtual. Otherwise depend on VIRTUAL attribute in the command
        if (foundVirtual && !found) {
            forceVirtual = true;
        }

        // inherit resource
        resourceId = parent.resourceId();
    } else {
        // deduce owning resource from current session id
        QString sessionId = QString::fromUtf8(connection()->sessionId());
        Resource res = Resource::retrieveByName(sessionId);
        if (!res.isValid()) {
            return failureResponse("Cannot create top-level collection");
        }
        resourceId = res.id();
    }

    Collection collection;
    if (parent.isValid()) {
        collection.setParentId(parent.id());
    }
    collection.setName(name);
    collection.setResourceId(resourceId);

    // attributes
    QList<QByteArray> attributes;
    QList<QByteArray> mimeTypes;
    QVector< QPair<QByteArray, QByteArray> > userDefAttrs;
    bool mimeTypesSet = false;
    attributes = m_streamParser->readParenthesizedList();
    for (int i = 0; i < attributes.count() - 1; i += 2) {
        const QByteArray key = attributes.at(i);
        const QByteArray value = attributes.at(i + 1);
        if (key == AKONADI_PARAM_REMOTEID) {
            collection.setRemoteId(QString::fromUtf8(value));
        } else if (key == AKONADI_PARAM_REMOTEREVISION) {
            collection.setRemoteRevision(QString::fromUtf8(value));
        } else if (key == AKONADI_PARAM_MIMETYPE) {
            ImapParser::parseParenthesizedList(value, mimeTypes);
            mimeTypesSet = true;
        } else if (key == AKONADI_PARAM_CACHEPOLICY) {
            HandlerHelper::parseCachePolicy(value, collection);
        } else if (key == AKONADI_PARAM_VIRTUAL) {
            collection.setIsVirtual(value.toUInt() != 0);
        } else if (key == AKONADI_PARAM_ENABLED) {
            collection.setEnabled(getTristateValue(value) == Server::True);
        } else if (key == AKONADI_PARAM_SYNC) {
            collection.setSyncPref(getTristateValue(value));
        } else if (key == AKONADI_PARAM_DISPLAY) {
            collection.setDisplayPref(getTristateValue(value));
        } else if (key == AKONADI_PARAM_INDEX) {
            collection.setIndexPref(getTristateValue(value));
        }
    }

    if (forceVirtual) {
        collection.setIsVirtual(true);
    }

    DataStore *db = connection()->storageBackend();
    Transaction transaction(db);

    if (!db->appendCollection(collection)) {
        return failureResponse("Could not create collection " + name.toLocal8Bit() + " resourceId: " + QByteArray::number(resourceId));
    }

    QStringList effectiveMimeTypes;
    if (mimeTypesSet) {
        Q_FOREACH (const QByteArray &b, mimeTypes) {
            effectiveMimeTypes << QString::fromUtf8(b);
        }
    } else {
        Q_FOREACH (const MimeType &mt, parentContentTypes) {
            effectiveMimeTypes << mt.name();
        }
    }
    if (!db->appendMimeTypeForCollection(collection.id(), effectiveMimeTypes)) {
        return failureResponse("Unable to append mimetype for collection " + name.toLocal8Bit() + " resourceId: " + QByteArray::number(resourceId));
    }

    // store user defined attributes
    typedef QPair<QByteArray, QByteArray> QByteArrayPair;
    Q_FOREACH (const QByteArrayPair &attr, userDefAttrs) {
        if (!db->addCollectionAttribute(collection, attr.first, attr.second)) {
            return failureResponse("Unable to add collection attribute.");
        }
    }

    Response response;
    response.setUntagged();

    // write out collection details
    db->activeCachePolicy(collection);
    const QByteArray b = HandlerHelper::collectionToByteArray(collection);
    response.setString(b);
    Q_EMIT responseAvailable(response);

    if (!transaction.commit()) {
        return failureResponse("Unable to commit transaction.");
    }

    return successResponse("CREATE completed");
}
