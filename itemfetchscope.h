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
  job->fetchScope().setFetchAllParts( true );
  @endcode

  Example: @c replacing an ItemFetchJob's scope
  @code
  Akonadi::ItemFetchScope scope;
  scope.setAllFetchAllParts( true );

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
    void addFetchPart( const QString &identifier );
    //FIXME_API:(volker) split into fetchPayloadPart(id, enum),
    //                   fetchAttribute(attr) and fetchAllAttributes()
    // make part identifier a QByteArray
    // move methods into ItemFetchScope
    // add method setItemFetchScope(ifs) and 'ItemFetchScope &itemFetchScope() const'

    /**
      Returns which item parts sould be fetched.

      @return a list of item part identifiers added through addFetchPart()

      @see fetchAllParts()
    */
    QStringList fetchPartList() const;

    /**
      Fetch all item parts.

      @param fetchAll if @c true all item parts should be fetched independent of
             the parts specified in fetchPartList()

      @see fetchAllParts()
    */
    void setFetchAllParts( bool fetchAll );
    //FIXME_API:(volker) rename to fetchAllPayloadParts(enum)

    /**
      Returns whether all parts sould be fetched.

      @return @c true if all available item parts should be fetched,
              @c false if only the parts specified by fetchPartList() should be fetched

      @see setFetchAllParts()
    */
    bool fetchAllParts() const;

  private:
    //@cond PRIVATE
    QSharedDataPointer<ItemFetchScopePrivate> d;
    //@endcond
};

}

#endif
