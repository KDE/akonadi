/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>
                  2007 Till Adam <adam@kde.org>

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

#include "akonadi_export.h"

#include <akonadi/entity.h>

#include <QtCore/QByteArray>
#include <QtCore/QMetaType>
#include <QtCore/QSet>

#include <typeinfo>

class KUrl;
class QStringList;

namespace Akonadi {

class ItemPrivate;

#include "itempayloadinternals_p.h"

/**
  This class represents a PIM item stored in Akonadi.

  A PIM item consists of one or more parts, allowing a fine-grained access on its
  content where needed (eg. mail envelope, mail body and attachments).

  There ise also a namespace (prefix) for special parts which are local to Akonadi.
  These parts, prefixed by "akonadi-" will never be fetched in the ressource.
  There are useful for local extensions like agents which might want to add metadata
  to items in order to handle them but the metadata should not be stored back to the
  ressource.

  This class contains beside some type-agnostic information (unique identifiers, flags)
  a single payload object representing its actual data. Which objects these actually
  are depends on the mimetype of the item and the corresponding serializer plugin.

  This class is implicitly shared.
*/
class AKONADI_EXPORT Item : public Entity
{
  public:
    typedef QList<Item> List;

    typedef QByteArray Flag;
    typedef QSet<QByteArray> Flags;

    static const QLatin1String FullPayload;

    /**
     * Creates an invalid item.
     */
    Item();

    /**
     * Create a new item with the given unique @p id.
     */
    explicit Item( Id id );

    /**
      Create a new item with the given mimetype.
      @param mimeType The mimetype of this item.
    */
    explicit Item( const QString &mimeType );

    /**
     * Copy constructor.
     */
    Item( const Item &other );

    /**
      Destroys this item.
    */
    ~Item();

    /**
      Creates an item from the url
    */
    static Item fromUrl( const KUrl &url );

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
    void clearFlag( const QByteArray &name );

    /**
     * Overwrite the flags of the item by @p flags.
     */
    void setFlags( const Flags &flags );

    /**
     * Clear all flags from the item.
     */
    void clearFlags();

    /**
      Returns the full payload in its canonical representation, ie. the
      binary or textual format usually used for data with this mimetype.
      This is useful when communicating with non-Akonadi application by
      eg. drag&drop, copy&paste or stored files.
    */
    QByteArray payloadData() const;

    /**
      Sets the payload based on the canonical representation normally
      used for data of this mimetype.
      @param data The encoded data.
      @see fullPayloadData
    */
    void setPayloadFromData( const QByteArray &data );

    /**
      Returns the list of loaded payload parts. This is not necessarily the
      identical to all parts in the cache or to all available parts on the backend.
     */
    QStringList loadedPayloadParts() const;

    /**
      Returns the revision number of this item.
    */
    int revision() const;

    /**
      Sets the @p revision number of this item.
      Do not modify this value from within an application,
      it is updated automatically by the revision checking functions.
    */
    void setRevision( int revision );

    /**
      Returns the mime type of this item.
    */
    QString mimeType() const;

    /**
      Sets the mime type of this item to @p mimeType.
    */
    void setMimeType( const QString &mimeType );

    /**
      Sets the payload object of this PIM item.
      The payload MUST NOT be a pointer, use a boost::shared_ptr instead.
      The payload should be an implicitly shared class.
    */
    template <typename T>
    void setPayload( T p )
    {
      setPayloadBase( new Payload<T>( p ) );
    }

    /**
      Returns the payload object of this PIM item.
      This method will abort if you try to retrieve the wrong payload type.
    */
    template <typename T>
    T payload() const
    {
        if ( !payloadBase() ) Q_ASSERT_X(false, "Akonadi::Item::payload()", "No valid payload set.");

        Payload<T> *p = dynamic_cast<Payload<T>*>( payloadBase() );
        // try harder to cast, workaround for some gcc issue with template instances in multiple DSO's
        if ( !p && strcmp( payloadBase()->typeName(), typeid(p).name() ) == 0 ) {
          p = reinterpret_cast<Payload<T>*>( payloadBase() );
        }
        if ( !p )
          qFatal( "Akonadi::Item::payload(): Wrong payload type (is '%s', requested '%s')",
                  payloadBase()->typeName(), typeid(p).name() );
        return p->payload;
    }

    /**
      Returns true if this item has a payload object.
    */
    bool hasPayload() const;

    /**
      Returns true if this item has a payload of type @c T.
    */
    template <typename T>
    bool hasPayload() const
    {
        if ( !hasPayload() )
          return false;
        Payload<T> *p = dynamic_cast<Payload<T>*>( payloadBase() );
        // try harder to cast, workaround for some gcc issue with template instances in multiple DSO's
        if ( !p && strcmp( payloadBase()->typeName(), typeid(p).name() ) == 0 ) {
          p = reinterpret_cast<Payload<T>*>( payloadBase() );
        }
        return p;
    }

    /**
     * Describes the type of url which is returned in url().
     */
    enum UrlType
    {
      UrlShort = 0,         ///< A short url which contains the identifier only (default)
      UrlWithMimeType = 1   ///< A url with identifier and mimetype
    };

    /**
      Returns the url for the item
     */
    KUrl url( UrlType type = UrlShort ) const;

  private:
    friend class ItemModifyJob;
    friend class ItemFetchJob;
    PayloadBase* payloadBase() const;
    void setPayloadBase( PayloadBase* );
    AKONADI_DECLARE_PRIVATE( Item )
};

}

Q_DECLARE_METATYPE(Akonadi::Item)
Q_DECLARE_METATYPE(Akonadi::Item::List)

#endif
