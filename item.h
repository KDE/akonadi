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

namespace Akonadi {

class ItemPrivate;

#include "itempayloadinternals_p.h"

/**
 * @short Represents a PIM item stored in Akonadi storage.
 *
 * A PIM item consists of one or more parts, allowing a fine-grained access on its
 * content where needed (eg. mail envelope, mail body and attachments).
 *
 * There is also a namespace (prefix) for special parts which are local to Akonadi.
 * These parts, prefixed by "akonadi-" will never be fetched in the resource.
 * They are useful for local extensions like agents which might want to add meta data
 * to items in order to handle them but the meta data should not be stored back to the
 * resource.
 *
 * This class contains beside some type-agnostic information (flags, revision)
 * a single payload object representing its actual data. Which objects these actually
 * are depends on the mimetype of the item and the corresponding serializer plugin.
 *
 * This class is implicitly shared.
 *
 * @author Volker Krause <vkrause@kde.org>, Till Adam <adam@kde.org>
 */
class AKONADI_EXPORT Item : public Entity
{
  public:
    /**
     * Describes a list of items.
     */
    typedef QList<Item> List;

    /**
     * Describes a flag name.
     */
    typedef QByteArray Flag;

    /**
     * Describes a set of flag names.
     */
    typedef QSet<QByteArray> Flags;

    /**
     * Describes the part name that is used to fetch the
     * full payload of an item.
     */
    static const char* FullPayload;

    /**
     * Creates a new item.
     */
    Item();

    /**
     * Creates a new item with the given unique @p id.
     */
    explicit Item( Id id );

    /**
     * Creates a new item with the given mime type.
     *
     * @param mimeType The mime type of the item.
     */
    explicit Item( const QString &mimeType );

    /**
     * Creates a new item from an @p other item.
     */
    Item( const Item &other );

    /**
     * Destroys the item.
     */
    ~Item();

    /**
     * Creates an item from the given @p url.
     */
    static Item fromUrl( const KUrl &url );

    /**
     * Returns all flags of this item.
     */
    Flags flags() const;

    /**
     * Returns whether the flag with the given @p name is
     * set in the item.
     */
    bool hasFlag( const QByteArray &name ) const;

    /**
     * Sets the flag with the given @p name in the item.
     */
    void setFlag( const QByteArray &name );

    /**
     * Removes the flag with the given @p name from the item.
     */
    void clearFlag( const QByteArray &name );

    /**
     * Overwrites all flags of the item by the given @p flags.
     */
    void setFlags( const Flags &flags );

    /**
     * Removes all flags from the item.
     */
    void clearFlags();

    /**
     * Sets the payload based on the canonical representation normally
     * used for data of this mime type.
     *
     * @param data The encoded data.
     * @see fullPayloadData
     */
    void setPayloadFromData( const QByteArray &data );

    /**
     * Returns the full payload in its canonical representation, e.g. the
     * binary or textual format usually used for data with this mime type.
     * This is useful when communicating with non-Akonadi application by
     * e.g. drag&drop, copy&paste or stored files.
     */
    QByteArray payloadData() const;

    /**
     * Returns the list of loaded payload parts. This is not necessarily
     * identical to all parts in the cache or to all available parts on the backend.
     */
    QSet<QByteArray> loadedPayloadParts() const;

    /**
     * Sets the @p revision number of the item.
     *
     * @note Do not modify this value from within an application,
     * it is updated automatically by the revision checking functions.
     */
    void setRevision( int revision );

    /**
     * Returns the revision number of the item.
     */
    int revision() const;

    /**
     * Sets the mime type of the item to @p mimeType.
     */
    void setMimeType( const QString &mimeType );

    /**
     * Returns the mime type of the item.
     */
    QString mimeType() const;

    /**
     * Sets the payload object of this PIM item.
     *
     * The payload MUST NOT be a pointer, use a boost::shared_ptr instead.
     * The payload should be an implicitly shared class.
     */
    template <typename T>
    void setPayload( T p )
    {
      setPayloadBase( new Payload<T>( p ) );
    }

    /**
     * Returns the payload object of this PIM item.
     *
     * @note This method will abort the application if you try to
     *       retrieve the wrong payload type, so better always check
     *       the mime type first.
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
     * Returns whether the item has a payload object.
     */
    bool hasPayload() const;

    /**
     * Returns whether the item has a payload of type @c T.
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
     * Returns the url of the item.
     */
    KUrl url( UrlType type = UrlShort ) const;

  private:
    //@cond PRIVATE
    friend class ItemModifyJob;
    friend class ItemFetchJob;
    PayloadBase* payloadBase() const;
    void setPayloadBase( PayloadBase* );
    //@endcond

    AKONADI_DECLARE_PRIVATE( Item )
};

}

Q_DECLARE_METATYPE(Akonadi::Item)
Q_DECLARE_METATYPE(Akonadi::Item::List)

#endif
