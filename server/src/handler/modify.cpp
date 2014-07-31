/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "modify.h"

#include <connection.h>
#include <storage/datastore.h>
#include <storage/entity.h>
#include <storage/transaction.h>
#include "libs/imapparser_p.h"
#include "imapstreamparser.h"
#include <handlerhelper.h>
#include <response.h>
#include <storage/itemretriever.h>
#include <storage/selectquerybuilder.h>
#include <storage/collectionqueryhelper.h>
#include <libs/protocol_p.h>
#include <search/searchmanager.h>
#include <akdebug.h>
#include <cachecleaner.h>
#include <collectionreferencemanager.h>
#include <akonadi.h>
#include <intervalcheck.h>

using namespace Akonadi;
using namespace Akonadi::Server;

Modify::Modify(Scope::SelectionScope scope)
    : m_scope(scope)
{
}

static Tristate getTristateValue(const QByteArray &line,  int &pos)
{
    QByteArray tmp;
    pos = ImapParser::parseString(line, tmp, pos);
    if (tmp == "TRUE") {
        return Tristate::True;
    } else if (tmp == "FALSE") {
        return Tristate::False;
    }
    return Tristate::Undefined;
}

bool Modify::parseStream()
{
    m_scope.parseScope(m_streamParser);
    SelectQueryBuilder<Collection> qb;
    CollectionQueryHelper::scopeToQuery(m_scope, connection(), qb);
    if (!qb.exec()) {
        throw HandlerException("Unable to execute collection query");
    }
    const Collection::List collections = qb.result();
    if (collections.isEmpty()) {
        throw HandlerException("No such collection");
    }
    if (collections.size() > 1) {   // TODO
        throw HandlerException("Mass-modifying collections is not yet implemented");
    }
    Collection collection = collections.first();

    //TODO: do it cleanly with the streaming parser, which doesn't have look-ahead at this moment
    QByteArray line = m_streamParser->readUntilCommandEnd();
    m_streamParser->insertData("\n");

    CacheCleanerInhibitor inhibitor(false);

    int p = 0;
    if ((p = line.indexOf(AKONADI_PARAM_PARENT)) > 0) {
        QByteArray tmp;
        ImapParser::parseString(line, tmp, p + 6);
        const Collection newParent = HandlerHelper::collectionFromIdOrName(tmp);
        if (newParent.isValid() && collection.parentId() != newParent.id()
            && collection.resourceId() != newParent.resourceId()) {
            inhibitor.inhibit();
            ItemRetriever retriever(connection());
            retriever.setCollection(collection, true);
            retriever.setRetrieveFullPayload(true);
            if (!retriever.exec()) {
                throw HandlerException(retriever.lastError());
            }
        }
    }

    DataStore *db = connection()->storageBackend();
    Transaction transaction(db);
    QList<QByteArray> changes;

    int pos = 0;
    while (pos < line.length()) {
        QByteArray type;
        pos = ImapParser::parseString(line, type, pos);
        if (type == AKONADI_PARAM_MIMETYPE) {
            QList<QByteArray> mimeTypes;
            pos = ImapParser::parseParenthesizedList(line, mimeTypes, pos);
            QStringList mts;
            Q_FOREACH (const QByteArray &ba, mimeTypes) {
                mts << QString::fromLatin1(ba);
            }
            MimeType::List currentMts = collection.mimeTypes();
            bool equal = true;
            Q_FOREACH (const MimeType &currentMt, currentMts) {
                if (mts.contains(currentMt.name())) {
                    mts.removeAll(currentMt.name());
                    continue;
                }
                equal = false;
                if (!collection.removeMimeType(currentMt)) {
                    throw HandlerException("Unable to remove collection mimetype");
                }
            }
            if (!db->appendMimeTypeForCollection(collection.id(), mts)) {
                return failureResponse("Unable to add collection mimetypes");
            }
            if (!equal || !mts.isEmpty()) {
                changes.append(AKONADI_PARAM_MIMETYPE);
            }
        } else if (type == AKONADI_PARAM_CACHEPOLICY) {
            bool changed = false;
            pos = HandlerHelper::parseCachePolicy(line, collection, pos, &changed);
            if (changed) {
                changes.append(AKONADI_PARAM_CACHEPOLICY);
            }
        } else if (type == AKONADI_PARAM_NAME) {
            QString newName;
            pos = ImapParser::parseString(line, newName, pos);
            if (collection.name() == newName) {
                continue;
            }
            if (!CollectionQueryHelper::hasAllowedName(collection, newName, collection.parentId())) {
                throw HandlerException("Collection with the same name exists already");
            }
            collection.setName(newName);
            changes.append(AKONADI_PARAM_NAME);
        } else if (type == AKONADI_PARAM_PARENT) {
            qint64 newParent;
            bool ok = false;
            pos = ImapParser::parseNumber(line, newParent, &ok, pos);
            if (!ok) {
                return failureResponse("Invalid syntax: " + line);
            }
            if (collection.parentId() == newParent) {
                continue;
            }
            if (!db->moveCollection(collection, Collection::retrieveById(newParent))) {
                return failureResponse("Unable to reparent collection");
            }
            changes.append(AKONADI_PARAM_PARENT);
        } else if (type == AKONADI_PARAM_VIRTUAL) {
            QString newValue;
            pos = ImapParser::parseString(line, newValue, pos);
            if (newValue.toInt() != collection.isVirtual()) {
                return failureResponse("Can't modify VIRTUAL collection flag");
            }
        } else if (type == AKONADI_PARAM_REMOTEID) {
            QString rid;
            pos = ImapParser::parseString(line, rid, pos);
            if (rid == collection.remoteId()) {
                continue;
            }
            if (!connection()->isOwnerResource(collection)) {
                throw HandlerException("Only resources can modify remote identifiers");
            }
            collection.setRemoteId(rid);
            changes.append(AKONADI_PARAM_REMOTEID);
        } else if (type == AKONADI_PARAM_REMOTEREVISION) {
            QString remoteRevision;
            pos = ImapParser::parseString(line, remoteRevision, pos);
            if (remoteRevision == collection.remoteRevision()) {
                continue;
            }
            collection.setRemoteRevision(remoteRevision);
            changes.append(AKONADI_PARAM_REMOTEREVISION);
        } else if (type == AKONADI_PARAM_PERSISTENTSEARCH) {
            QByteArray tmp;
            QList<QByteArray> queryArgs;
            pos = ImapParser::parseString(line, tmp, pos);
            ImapParser::parseParenthesizedList(tmp, queryArgs);
            QString queryString, queryCollections, queryAttributes;
            QStringList attrs;
            for (int i = 0; i < queryArgs.size(); ++i) {
                const QByteArray key = queryArgs.at(i);
                if (key == AKONADI_PARAM_PERSISTENTSEARCH_QUERYSTRING) {
                    queryString = QString::fromUtf8(queryArgs.at(i + 1));
                    ++i;
                } else if (key == AKONADI_PARAM_PERSISTENTSEARCH_QUERYCOLLECTIONS) {
                    QList<QByteArray> cols;
                    ImapParser::parseParenthesizedList(queryArgs.at(i +  1), cols);
                    queryCollections = QString::fromLatin1(ImapParser::join(cols, " "));
                    ++i;
                } else if (key == AKONADI_PARAM_PERSISTENTSEARCH_QUERYLANG) {
                    // Ignore query lang
                    ++i;
                } else if (key == AKONADI_PARAM_REMOTE) {
                    attrs << QString::fromLatin1(key);
                } else if (key == AKONADI_PARAM_RECURSIVE) {
                    attrs << QString::fromLatin1(key);
                }
            }

            queryAttributes = attrs.join(QLatin1String(" "));

            qDebug() << collection.queryAttributes() << queryAttributes;
            qDebug() << collection.queryCollections() << queryCollections;
            qDebug() << collection.queryString() << queryString;

            if (collection.queryAttributes() != queryAttributes
                || collection.queryCollections() != queryCollections
                || collection.queryString() != queryString
                || changes.contains(AKONADI_PARAM_MIMETYPE)) {
                collection.setQueryString(queryString);
                collection.setQueryCollections(queryCollections);
                collection.setQueryAttributes(queryAttributes);

                SearchManager::instance()->updateSearch(collection);
                changes.append(AKONADI_PARAM_PERSISTENTSEARCH);
            }
        } else if (type == AKONADI_PARAM_ENABLED) {
            //Not actually a tristate
            const bool enabled = (getTristateValue(line, pos) == Tristate::True);
            if (enabled != collection.enabled()) {
                collection.setEnabled(enabled);
                changes.append(AKONADI_PARAM_ENABLED);
            }
        } else if (type == AKONADI_PARAM_SYNC) {
            const Tristate tristate = getTristateValue(line, pos);
            if (tristate != collection.syncPref()) {
                collection.setSyncPref(tristate);
                changes.append(AKONADI_PARAM_SYNC);
            }
        } else if (type == AKONADI_PARAM_DISPLAY) {
            const Tristate tristate = getTristateValue(line, pos);
            if (tristate != collection.displayPref()) {
                collection.setDisplayPref(tristate);
                changes.append(AKONADI_PARAM_DISPLAY);
            }
        } else if (type == AKONADI_PARAM_INDEX) {
            const Tristate tristate = getTristateValue(line, pos);
            if (tristate != collection.indexPref()) {
                collection.setIndexPref(tristate);
                changes.append(AKONADI_PARAM_INDEX);
            }
        } else if (type == AKONADI_PARAM_REFERENCED) {
            //Not actually a tristate
            const bool reference = (getTristateValue(line, pos) == Tristate::True);
            connection()->collectionReferenceManager()->referenceCollection(connection()->sessionId(), collection, reference);
            const bool referenced = connection()->collectionReferenceManager()->isReferenced(collection.id());
            if (referenced != collection.referenced()) {
                collection.setReferenced(referenced);
                if (AkonadiServer::instance()->intervalChecker() && referenced) {
                    AkonadiServer::instance()->intervalChecker()->requestCollectionSync(collection);
                }
                changes.append(AKONADI_PARAM_REFERENCED);
            }
        } else if (type.isEmpty()) {
            break; // input end
        } else {
            // custom attribute
            if (type.startsWith('-')) {
                type = type.mid(1);
                if (db->removeCollectionAttribute(collection, type)) {
                    changes.append(type);
                }
            } else {
                QByteArray value;
                pos = ImapParser::parseString(line, value, pos);

                SelectQueryBuilder<CollectionAttribute> qb;
                qb.addValueCondition(CollectionAttribute::collectionIdColumn(), Query::Equals, collection.id());
                qb.addValueCondition(CollectionAttribute::typeColumn(), Query::Equals, type);
                if (!qb.exec()) {
                    throw HandlerException("Unable to retrieve collection attribute");
                }

                const CollectionAttribute::List attrs = qb.result();
                if (attrs.isEmpty()) {
                    CollectionAttribute attr;
                    attr.setCollectionId(collection.id());
                    attr.setType(type);
                    attr.setValue(value);
                    if (!attr.insert()) {
                        throw HandlerException("Unable to add collection attribute");
                    }
                    changes.append(type);
                } else if (attrs.size() == 1) {
                    CollectionAttribute attr = attrs.first();
                    if (attr.value() == value) {
                        continue;
                    }
                    attr.setValue(value);
                    if (!attr.update()) {
                        throw HandlerException("Unable to update collection attribute");
                    }
                    changes.append(type);
                } else {
                    throw HandlerException("WTF: more than one attribute with the same name");
                }
            }
        }
    }

    if (!changes.isEmpty()) {
        if (collection.hasPendingChanges() && !collection.update()) {
            return failureResponse("Unable to update collection");
        }
        db->notificationCollector()->collectionChanged(collection, changes);
        //For backwards compatibility. Must be after the changed notification (otherwise the compression removes it).
        if (changes.contains(AKONADI_PARAM_ENABLED)) {
            if (collection.enabled()) {
                db->notificationCollector()->collectionSubscribed(collection);
            } else {
                db->notificationCollector()->collectionUnsubscribed(collection);
            }
        }
        if (!transaction.commit()) {
            return failureResponse("Unable to commit transaction");
        }
    }

    Response response;
    response.setTag(tag());
    response.setString("MODIFY done");
    Q_EMIT responseAvailable(response);
    return true;
}
