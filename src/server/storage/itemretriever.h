/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef ITEMRETRIEVER_H
#define ITEMRETRIEVER_H

#include <QObject>
#include <QDateTime>

#include "../exception.h"
#include "entities.h"

#include <private/scope_p.h>
#include <private/imapset_p.h>


AKONADI_EXCEPTION_MAKE_INSTANCE(ItemRetrieverException);

namespace Akonadi
{
namespace Server
{

class Connection;
class CommandContext;

/**
  Helper class for retrieving missing items parts from remote resources.

  Stuff in here happens in the calling thread and does not access shared data.

  @todo make usable for Fetch by allowing to share queries
*/
class ItemRetriever : public QObject
{
    Q_OBJECT

public:
    explicit ItemRetriever(Connection *connection, const CommandContext &context);

    Connection *connection() const;

    void setRetrieveParts(const QVector<QByteArray> &parts);
    QVector<QByteArray> retrieveParts() const;
    void setRetrieveFullPayload(bool fullPayload);
    void setChangedSince(const QDateTime &changedSince);
    void setItemSet(const ImapSet &set, const Collection &collection = Collection());
    void setItemSet(const ImapSet &set, bool isUid);
    void setItem(const Entity::Id &id);
    /** Retrieve all items in the given collection. */
    void setCollection(const Collection &collection, bool recursive = true);

    /** Retrieve all items matching the given item scope. */
    void setScope(const Scope &scope);
    Scope scope() const;

    bool exec();

    QByteArray lastError() const;

Q_SIGNALS:
    void itemsRetrieved(const QList<qint64> &ids);

private:
    QSqlQuery buildQuery() const;

    /**
     * Checks if external files are still present
     * This costs extra, but allows us to automatically recover from something changing the external file storage.
     */
    void verifyCache();

    Akonadi::ImapSet mItemSet;
    Collection mCollection;
    Scope mScope;
    Connection *mConnection = nullptr;
    const CommandContext &mContext;
    QVector<QByteArray> mParts;
    bool mFullPayload;
    bool mRecursive;
    QDateTime mChangedSince;
    mutable QByteArray mLastError;
    bool mCanceled;
};

} // namespace Server
} // namespace Akonadi

#endif
