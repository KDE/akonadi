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
#include <QtCore/QDebug>
#include <QtCore/QSet>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QStringList>

#include <typeinfo>

namespace Akonadi {

#include "itempayloadinternals_p.h"

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
    explicit Item( const DataReference &ref = DataReference() );

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
      Adds a new part with the given @p identifier and @p data.
      If a part with the identifier already exists, it is overwritten.
     */
    void addPart( const QString &identifier, const QByteArray &data );

    /**
      Returns the data of the part with the given @p identifier or any
      empty byte array if a part with the identifier doesn't exists.
     */
    QByteArray part( const QString &identifier ) const;

    /**
      Returns the list of part identifiers of this item.
     */
    QStringList availableParts() const;

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
    QString mimeType() const;

    /**
      Sets the mime type of this item to @p mimeType.
    */
    void setMimeType( const QString &mimeType );

    /**
      Assignment operator.
    */
    Item& operator=( const Item &other );

    /**
      Sets the payload object of this PIM item.
    */
    template <typename T>
    void setPayload( T p )
    {
            m_payload = new Payload<T>( p );
    }

    /**
      Returns the payload object of this PIM item.
    */
    template <typename T>
    T payload()
    {
        if ( !m_payload ) Q_ASSERT_X(false, "Akonadi::Item::payload()", "No valid payload set.");

        Payload<T> *p = dynamic_cast<Payload<T>*>(m_payload);
        // try harder to cast, workaround for some gcc issue with template instances in multiple DSO's
        if ( !p && strcmp( m_payload->typeName(), typeid(p).name() ) == 0 ) {
          p = reinterpret_cast<Payload<T>*>( m_payload );
        }
        if ( !p ) Q_ASSERT_X(false, "Akonadi::Item::payload()", "Wrong payload type.");
        return p->payload;
    }

    /**
      Returns the payload object of this PIM item.
    */
    template <typename T>
    const T payload() const
    {
        return const_cast<Item*>( this )->payload<T>();
    }

    /**
      Returns true if this item has a payload object.
    */
    bool hasPayload() const;

  private:
    class Private;
    QSharedDataPointer<Private> d;
    PayloadBase * m_payload;
};

}

Q_DECLARE_METATYPE(Akonadi::Item)

#endif
