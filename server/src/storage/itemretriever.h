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

#include "../exception.h"
#include "entities.h"
#include "handler/scope.h"

#include "libs/imapset_p.h"

#include <QStringList>

AKONADI_EXCEPTION_MAKE_INSTANCE( ItemRetrieverException );

namespace Akonadi {
  class AkonadiConnection;
  class QueryBuilder;

/**
  Helper class for retrieving missing items parts from remote resources.

  Stuff in here happens in the calling thread and does not access shared data.

  @todo make usable for Fetch by allowing to share queries
*/
class ItemRetriever
{
  public:
    ItemRetriever( AkonadiConnection *connection );
    ~ItemRetriever();

    AkonadiConnection *connection() const;

    void setRetrieveParts( const QStringList &parts );
    QStringList retrieveParts() const;
    void setRetrieveFullPayload( bool fullPayload );
    void setItemSet( const ImapSet &set, const Collection &collection = Collection() );
    void setItemSet( const ImapSet &set, bool isUid );
    void setItem( const Akonadi::Entity::Id &id );
    /** Retrieve all items in the given collection. */
    void setCollection( const Akonadi::Collection &collection, bool recursive = true );

    /** Retrieve all items matching the given item scope. */
    void setScope( const Scope &scope );
    Scope scope() const;

    void exec();

    /**
     * Returns the QueryBuilder containing the subset of the item query which is
     * not specific for the ItemRetriever for use by subclasses.
     */
    static QueryBuilder buildGenericItemQuery(const Akonadi::Scope& scope, Akonadi::AkonadiConnection* connection);

    /** Column indices of the generic item query. */
    enum GenericItemQueryColumns {
      GenericItemQueryPimItemIdColumn = 0,
      GenericItemQueryPimItemRidColumn = 1,
      GenericItemQueryMimeTypeColumn = 2,
      GenericItemQueryResourceColumn = 3
    };

    /**
     * Returns the QueryBuilder containing the subset of the part query which is
     * not specific for the ItemRetrieverQuery for use by subclasses.
     */
    static QueryBuilder buildGenericPartQuery();

    /** Column indices of the generic part query. */
    enum GenericPartQueryColumns {
      GenericPartQueryPimIdColumn = 0,
      GenericPartQueryNameColumn = 1,
      GenericPartQueryDataColumn = 2,
      GenericPartQueryExternalColumn = 3
    };

  private:
    /** Convenience method which returns the database driver name */
    QString driverName();

    QueryBuilder buildItemQuery() const;
    QueryBuilder buildPartQuery() const;


    ImapSet mItemSet;
    Collection mCollection;
    Scope mScope;
    AkonadiConnection* mConnection;
    QStringList mParts;
    bool mFullPayload;
    bool mRecursive;
};

}

#endif

