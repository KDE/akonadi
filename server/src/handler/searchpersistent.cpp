/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
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

#include "searchpersistent.h"

#include "akonadi.h"
#include "connection.h"
#include "response.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/transaction.h"
#include "handlerhelper.h"
#include "search/searchmanager.h"
#include "imapstreamparser.h"
#include "libs/protocol_p.h"
#include "libs/imapparser_p.h"
#include <akdebug.h>

#include <QtCore/QStringList>

using namespace Akonadi;
using namespace Akonadi::Server;

SearchPersistent::SearchPersistent()
    : Handler()
{
}

SearchPersistent::~SearchPersistent()
{
}

bool SearchPersistent::parseStream()
{
    QString collectionName = m_streamParser->readUtf8String();
    if (collectionName.isEmpty()) {
        return failureResponse("No name specified");
    }

    DataStore *db = connection()->storageBackend();
    Transaction transaction(db);

    const QString queryString = m_streamParser->readUtf8String();
    if (queryString.isEmpty()) {
        return failureResponse("No query specified");
    }

    // for legacy clients we have to guess the language
    QString lang = QLatin1String("SPARQL");

    QList<QByteArray> mimeTypes;
    QString queryCollections;
    QStringList queryAttributes;
    if (m_streamParser->hasList()) {
        m_streamParser->beginList();
        while (!m_streamParser->atListEnd()) {
            const QByteArray key = m_streamParser->readString();
            if (key == AKONADI_PARAM_MIMETYPE) {
                mimeTypes = m_streamParser->readParenthesizedList();
            } else if (key == AKONADI_PARAM_PERSISTENTSEARCH_QUERYCOLLECTIONS) {
                const QList<QByteArray> collections = m_streamParser->readParenthesizedList();
                queryCollections = QString::fromLatin1(ImapParser::join(collections, " "));
            } else if (key == AKONADI_PARAM_PERSISTENTSEARCH_QUERYLANG) {
                queryAttributes << QLatin1String(AKONADI_PARAM_PERSISTENTSEARCH_QUERYLANG)
                                << QString::fromUtf8(m_streamParser->readString());
            } else if (key == AKONADI_PARAM_REMOTE) {
                queryAttributes << QLatin1String(AKONADI_PARAM_REMOTE);
            } else if (key == AKONADI_PARAM_RECURSIVE) {
                queryAttributes << QLatin1String(AKONADI_PARAM_RECURSIVE);
            }
        }
    }

    Collection col;
    col.setQueryString(queryString);
    col.setQueryAttributes(queryAttributes.join(QLatin1String(" ")));
    col.setQueryCollections(queryCollections);
    col.setParentId(1);   // search root
    col.setResourceId(1);   // search resource
    col.setName(collectionName);
    col.setIsVirtual(true);
    if (!db->appendCollection(col)) {
        return failureResponse("Unable to create persistent search");
    }

    if (!db->addCollectionAttribute(col, "AccessRights", "luD")) {
        return failureResponse("Unable to set rights attribute on persistent search");
    }

    Q_FOREACH (const QByteArray &mimeType, mimeTypes) {
        col.addMimeType(MimeType::retrieveByName(QString::fromLatin1(mimeType)));
    }

    if (!transaction.commit()) {
        return failureResponse("Unable to commit transaction");
    }

    SearchManager::instance()->updateSearch(col);

    const QByteArray b = HandlerHelper::collectionToByteArray(col);

    Response colResponse;
    colResponse.setUntagged();
    colResponse.setString(b);
    Q_EMIT responseAvailable(colResponse);

    return successResponse("SEARCH_STORE completed");
}
