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

#ifndef AKONADI_COLLECTIONSTATUS_H
#define AKONADI_COLLECTIONSTATUS_H

#include "libakonadi_export.h"
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>

namespace Akonadi {

/**
  Contains status information of a collection, such as
  total number of items, number of new/unread items, etc.

  These information might be expensive to obtain and are thus
  not included when fetching collection with a CollectionListJob.
  They can be retrieved spearately using CollectionStatusJob.

  This class is implicitely shared.
*/
class AKONADI_EXPORT CollectionStatus
{
  public:
    /**
      Creates a new CollectionStatus object.
     */
    CollectionStatus();

    /**
      Copy constructor.
    */
    CollectionStatus( const CollectionStatus &other );

    /**
      Destructor.
    */
    ~CollectionStatus();

    /**
      Returns the number of objects in this collection.
      @return @c -1 if this information is not available.
      @see setCount()
      @see unreadCount()
     */
    int count() const;

    /**
      Sets the number of objects in this collection.
      @param count The number of objects
      @see count()
    */
    void setCount( int count );

    /**
      Returns the number of unread messages in this collection.
      @return @c -1 if this information is not available.
      @see setUnreadCount()
      @see count()
     */
    int unreadCount() const;

    /**
      Sets the number of unread messages in this collection.
      @param count The number of unread messages
      @see unreadCount()
    */
    void setUnreadCount( int count );

    /**
      Assignment operator.
      @param other The status object to assign to @c this
    */
    CollectionStatus& operator=( const CollectionStatus &other );

  private:
    class Private;
    QSharedDataPointer<Private> d;

};

}

Q_DECLARE_METATYPE(Akonadi::CollectionStatus)

#endif
