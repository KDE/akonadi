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

#ifndef PIM_MESSAGECOLLECTION_H
#define PIM_MESSAGECOLLECTION_H

#include <libakonadi/collection.h>

namespace PIM {

/**
  A collection of messages, eg. emails or news articles.
*/
class MessageCollection : public Collection
{
  public:
    /**
      Create a new collection.

      @param ref The data reference of this collection.
     */
    MessageCollection( const DataReference &ref );

    /**
      Copy constructor.

      @param other The MessageCollection to copy.
    */
    MessageCollection( const MessageCollection &other );

    /**
      Destroys this collection.
    */
    virtual ~MessageCollection();

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

  private:
    class Private;
    Private *d;

};

}

#endif
