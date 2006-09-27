/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#ifndef AKONADI_MESSAGECOLLECTIONATTRIBUTE_H
#define AKONADI_MESSAGECOLLECTIONATTRIBUTE_H

#include <libakonadi/collection.h>
#include <kdepim_export.h>

namespace Akonadi {

/**
  A collection of messages, eg. emails or news articles.
*/
class AKONADI_EXPORT MessageCollectionAttribute : public CollectionAttribute
{
  public:
    /**
      Create a new message collection attribute.
     */
    MessageCollectionAttribute();

    /**
      Destroys this collection attribute.
    */
    virtual ~MessageCollectionAttribute();

    /**
      Returns the number of objects in this collection.
      @returns -1 if this information is not available.
     */
    int count() const;

    /**
      Sets the number of objects in this collection.
    */
    void setCount( int count );

    /**
      Returns the number of unread messages in this collection.
      @returns -1 if this information is not available.
     */
    int unreadCount() const;

    /**
      Sets the number of unread messages in this collection.
    */
    void setUnreadCount( int count );

    virtual QByteArray type() const;

  private:
    class Private;
    Private *d;

};

}

#endif
