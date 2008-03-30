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

class AKONADI_EXPORT ItemFetchScope
{
  public:
    ItemFetchScope();

    ItemFetchScope( const ItemFetchScope &other );

    ~ItemFetchScope();

    ItemFetchScope &operator=( const ItemFetchScope &other );

    /**
      Choose which part(s) of the item shall be fetched.
    */
    void addFetchPart( const QString &identifier );
    //FIXME_API:(volker) split into fetchPayloadPart(id, enum),
    //                   fetchAttribute(attr) and fetchAllAttributes()
    // make part identifier a QByteArray
    // move methods into ItemFetchScope
    // add method setItemFetchScope(ifs) and 'ItemFetchScope &itemFetchScope() const'

    QStringList fetchPartList() const;

    /**
      Fetch all item parts.
    */
    void setFetchAllParts( bool fetchAll );
    //FIXME_API:(volker) rename to fetchAllPayloadParts(enum)

    bool fetchAllParts() const;

  private:
    //@cond PRIVATE
    QSharedDataPointer<ItemFetchScopePrivate> d;
    //@endcond
};

}

#endif
