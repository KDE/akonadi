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

#ifndef AKONADI_ITEM_H
#define AKONADI_ITEM_H

#include <libakonadi/job.h>

#include <QtCore/QByteArray>
#include <QtCore/QSet>
#include <QtCore/QSharedDataPointer>

namespace Akonadi {

/**
  Base class for all PIM items stored in Akonadi.
  It contains type-neutral data and the unique reference.
*/
class AKONADI_EXPORT Item
{
  public:
    typedef QList<Item> List;

    typedef QByteArray Flag;
    typedef QSet<QByteArray> Flags;

    /**
      Create a new PIM item.
      @param ref The unique reference of this item.
    */
    Item( const DataReference &ref = DataReference() );

    /**
     * Copy constructor.
     */
    Item( const Item &other );

    /**
      Destroys this PIM item.
    */
    virtual ~Item();

    /**
     * Returns whether the item is a valid PIM item.
     */
    bool isValid() const;

    /**
      Returns the DataReference of this item.
    */
    DataReference reference() const;

    /**
      Returns the flags of this item.
    */
    Flags flags() const;

    /**
      Checks if the given flag is set.
      @param name The flag name.
    */
    bool hasFlag( const QByteArray &name ) const;

    /**
      Sets an item flag.
      @param name The flag name.
    */
    void setFlag( const QByteArray &name );

    /**
      Removes an item flag.
      @param name The flag name.
    */
    void unsetFlag( const QByteArray &name );

    /**
      Returns the raw data of this item.
    */
    virtual QByteArray data() const;

    /**
      Sets the raw data of this item.
    */
    virtual void setData( const QByteArray& data );

    /**
      Returns the mime type of this item.
    */
    QByteArray mimeType() const;

    /**
      Sets the mime type of this item to @p mimeType.
    */
    void setMimeType( const QByteArray &mimeType );

    Item& operator=( const Item &other );

  private:
    class Private;
    QSharedDataPointer<Private> d;
};

}

#endif
