/*
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

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

#ifndef ITEMFETCHSCOPE_H
#define ITEMFETCHSCOPE_H

#include "akonadi_export.h"

#include <QtCore/QSharedDataPointer>

class QStringList;
template <typename T> class QSet;

namespace Akonadi {

class ItemFetchScopePrivate;

/**
  Specifies which parts of an item should be fetched from the Akonadi server.

  When items are fetched from server either by using ItemFetchJob explicitly or
  when it is being used internally by other classes, e.g. ItemModel, the scope
  of the fetch operation can be tailored to the application's current needs.

  There are two supported ways of changing the currently active ItemFetchScope
  of classes:
  - in-place: modify the ItemFetchScope object the other class holds as a member
  - replace: replace the other class' member with a new scope object

  Example: modifying an ItemFetchJob's scope @c in-place
  @code
  ItemFetchJob *job = new Akonadi::ItemFetchJob( collection );
  job->fetchScope().fetchFullPayload();
  job->fetchScope().fetchAttribute<MyAttribute>();
  @endcode

  Example: @c replacing an ItemFetchJob's scope
  @code
  Akonadi::ItemFetchScope scope;
  scope.fetchFullPayload();
  scope.fetchAttribute<MyAttribute>();

  ItemFetchJob *job = new Akonadi::ItemFetchJob( collection );
  job->setFetchScope( scope );
  @endcode
*/
class AKONADI_EXPORT ItemFetchScope
{
  public:
    /**
      Creates an empty scope.

      Using an empty scope will only fetch the very basic metadata of items,
      e.g. Akonadi ID, remote ID, MIME type
    */
    ItemFetchScope();

    /**
      Copies a given @p other scope.

      Since ItemFetchScope is an implicitly shared object, this is a cheap operation
    */
    ItemFetchScope( const ItemFetchScope &other );

    /**
      Destroys the scope instance
    */
    ~ItemFetchScope();

    /**
      Copies a given @p other scope.

      Since ItemFetchScope is an implicitly shared object, this is a cheap operation
    */
    ItemFetchScope &operator=( const ItemFetchScope &other );

    /**
      Choose which part(s) of the item shall be fetched.

      @param identifier identifier of an item part to be fetched

      @see fetchPartList()
    */
    KDE_DEPRECATED void addFetchPart( const QString &identifier ); //FIXME_API: kill me

    /**
      Returns which item parts sould be fetched.

      @return a list of item part identifiers added through addFetchPart()

      @see fetchAllParts()
    */
    KDE_DEPRECATED QStringList fetchPartList() const; //FIXME_API: kill me

    /**
      Fetch all item parts.

      @param fetchAll if @c true all item parts should be fetched independent of
             the parts specified in fetchPartList()

      @see fetchAllParts()
    */
    KDE_DEPRECATED void setFetchAllParts( bool fetchAll ); //FIXME_API: kill me

    /**
      Returns whether all parts sould be fetched.

      @return @c true if all available item parts should be fetched,
              @c false if only the parts specified by fetchPartList() should be fetched

      @see setFetchAllParts()
    */
    KDE_DEPRECATED bool fetchAllParts() const; //FIXME_API: kill me


    /**
      Returns the payload parts that should be fetched.
      @see fetchPayloadPart()
    */
    QSet<QByteArray> payloadParts() const;

    /**
      Select which payload parts you want to retrieve.
      @param part The payload part identifier. Valid values depend
      on the item type.
      @param fetch @c true to fetch this part, @c false otherwise.
    */
    void fetchPayloadPart( const QByteArray &part, bool fetch = true );

    KDE_DEPRECATED void fetchPayloadPart( const QString &part, bool fetch = true ); // FIXME_API: make part identifiers QByteArrays

    /**
      Returns whether the full payload should be retrieved.
      @see fetchFullPayload()
    */
    bool fullPayload() const;

    /**
      Select if you want to fetch the full payload.
      @param fetch @c true if the full payload should be fetched, @c false otherwise.
    */
    void fetchFullPayload( bool fetch = true );

    /**
      Returns all explicitly fetched attributes.
      Undefined if fetchAllAttributes() returns true.
      @see fetchAttribute()
    */
    QSet<QByteArray> attributes() const;

    /**
      Select if attribute of type @p type should be fetched.
      @param type The attribute type to fetch.
      @param fetch Select if this attribute should be fetched or not.
    */
    void fetchAttribute( const QByteArray &type, bool fetch = true );

    /**
      Selects if attributes of type @p T should be fetched.
      @param fetch Select if this attribute should be fetched or not.
    */
    template <typename T> inline void fetchAttribute( bool fetch = true )
    {
      T dummy;
      fetchAttribute( dummy.type(), fetch );
    }

    /**
      Returns whether all available attributes should be fetched.
      @see fetchAllAttributes()
    */
    bool allAttributes() const;

    /**
      Select whether or not all available item attributes should be fetched.
      @param fetch Select if all attributes should be fetched or not.
    */
    void fetchAllAttributes( bool fetch = true );

    /**
      Returns whether payload data should be requested from remote sources or just
      from the local cache.
      @see setCacheOnly()
    */
    bool cacheOnly() const;

    /**
      Select whether payload data should be requested from remote sources or just
      from the local cache.
      @param chacheOnly @c true if no remote data should be requested,
      @c false otherwise (the default).
    */
    void setCacheOnly( bool cacheOnly );

    /**
      Returns @c true if there is nothing to fetch.
    */
    bool isEmpty() const;

  private:
    //@cond PRIVATE
    QSharedDataPointer<ItemFetchScopePrivate> d;
    //@endcond
};

}

#endif
