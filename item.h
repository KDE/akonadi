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
#include <akonadi/exception.h>
#include "itempayloadinternals_p.h"

#include <QtCore/QByteArray>
#include <QtCore/QMetaType>
#include <QtCore/QSet>

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <typeinfo>

class KUrl;

namespace std {
  template <typename T> class auto_ptr;
}

namespace Akonadi {

class ItemPrivate;

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
 * <h4>Payload</h4>
 *
 * Technically the only restriction on payload objects is that they have to be copyable.
 * For safety reasons, pointer payloads are forbidden as well though, as the
 * ownership would not be clear. In this case, usage of a shared pointer is
 * recommended (such as boost::shared_ptr or QSharedPointer).
 *
 * Using a shared pointer is also recommended in case the payload uses polymorphic
 * types. For supported shared pointer types implicit casting is provided when possible.
 *
 * When using a value-based class as payload, it is recommended to use one that does
 * support implicit sharing as setting and retrieving a payload as well as copying
 * an Akonadi::Item object imply copying of the payload object.
 *
 * The availability of a payload of a specific type can be checked using hasPayload(),
 * payloads can be retrieved by using payload() and set by using setPayload(). Refer
 * to the documentation of those methods for more details.
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
     * Returns the timestamp of the last modification of this item.
     * @since 4.2
     */
    QDateTime modificationTime() const;

    /**
     * Sets the timestamp of the last modification of this item.
     *
     * @note Do not modify this value from within an application,
     * it is updated automatically by the revision checking functions.
     * @since 4.2
     */
    void setModificationTime( const QDateTime &datetime );

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
     * Returns the unique identifier of the collection this item is stored in. There is only
     * a single such collection, although the item can be linked into arbitrary many
     * virtual collections.
     * Calling this method makes sense only after running an ItemFetchJob on the item.
     * @returns the collection ID if it is known, -1 otherwise.
     * @since 4.3
     */
    Entity::Id storageCollectionId() const;

    /**
     * Set the size of the item in bytes.
     *
     * @since 4.2
     */
    void setSize( qint64 size );

    /**
     * Returns the size of the items in bytes.
     *
     * @since 4.2
     */
    qint64 size() const;

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
     * @param p The payload object. Must be copyable and must not be a pointer,
     * will cause a compilation failure otherwise. Using a type that can be copied
     * fast (such as implicitly shared classes) is recommended.
     * If the payload type is polymorphic and you intend to set and retrieve payload
     * objects with mismatching but castable types, make sure to use a supported
     * shared pointer implementation (currently boost::shared_ptr and QSharedPointer)
     * and make sure there is a specialization of Akonadi::super_trait for your class.
     */
    template <typename T> void setPayload( const T &p );
    //@cond PRIVATE
    template <typename T> void setPayload( T* p );
    template <typename T> void setPayload( std::auto_ptr<T> p );
    //@endcond

    /**
     * Returns the payload object of this PIM item. This method will only succeed if either
     * you requested the exact same payload type that was put in or the payload uses a
     * supported shared pointer type (currently boost::shared_ptr and QSharedPointer), and
     * is castable to the requested type. For this to work there needs to be a specialization
     * of Akonadi::super_trait of the used classes.
     *
     * If a mismatching or non-castable payload type is requested, an Akonadi::PayloadException
     * is thrown. Therefore it is generally recommended to guard calls to payload() with a
     * corresponding hasPayload() call.
     *
     * Trying to retrieve a pointer type will fail to compile.
     */
    template <typename T> T payload() const;

    /**
     * Returns whether the item has a payload object.
     */
    bool hasPayload() const;

    /**
     * Returns whether the item has a payload of type @c T.
     * This method will only return @c true if either you requested the exact same payload type
     * that was put in or the payload uses a supported shared pointer type (currently boost::shared_ptr
     * and QSharedPointer), and is castable to the requested type. For this to work there needs
     * to be a specialization of Akonadi::super_trait of the used classes.
     *
     * Trying to retrieve a pointer type will fail to compile.
     */
    template <typename T> bool hasPayload() const;

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

    /**
     * Returns the parts available for this item.
     *
     * The returned set refers to parts available on the akonadi server or remotely,
     * but does not include the loadedPayloadParts() of this item.
     *
     * @since 4.4
     */
    QSet<QByteArray> availablePayloadParts() const;

    /**
     * Applies the parts of Item @p other to this item.
     * Any parts or attributes available in other, will be applied to this item,
     * and the payload parts of other will be inserted into this item, overwriting
     * any existing parts with the same part name.
     *
     * If there is an ItemSerialzerPluginV2 for the type, the merge method in that plugin is
     * used to perform the merge. If only an ItemSerialzerPlugin class is found, or the merge
     * method of the -V2 plugin is not implemented, the merge is performed with multiple deserializations
     * of the payload.
     *
     * @since 4.4
     */
    void apply( const Item &other );

  private:
    //@cond PRIVATE
    friend class ItemCreateJob;
    friend class ItemModifyJob;
    friend class ProtocolHelper;
    PayloadBase* payloadBase() const;
    void setPayloadBase( PayloadBase* );
    /**
     * Set the collection ID to where the item is stored in. Should be set only by the ItemFetchJob.
     * @param collectionId the unique identifier of the the collection where this item is stored in.
     * @since 4.3
     */
    void setStorageCollectionId( Entity::Id collectionId);

    //@endcond

    AKONADI_DECLARE_PRIVATE( Item )
};


template <typename T>
T Item::payload() const
{
  BOOST_STATIC_ASSERT( !boost::is_pointer<T>::value );

  if ( !payloadBase() )
    throw PayloadException( "No payload set" );

  typedef Internal::PayloadTrait<T> PayloadType;
  if ( PayloadType::isPolymorphic ) {
    try {
      const typename PayloadType::SuperType sp = payload<typename PayloadType::SuperType>();
      return PayloadType::castFrom( sp );
    } catch ( const Akonadi::PayloadException& ) {}
  }

  Payload<T> *p = Internal::payload_cast<T>( payloadBase() );
  if ( !p ) {
    throw PayloadException( QString::fromLatin1( "Wrong payload type (is '%1', requested '%2')" )
      .arg( QLatin1String( payloadBase()->typeName() ) )
      .arg( QLatin1String( typeid(p).name() ) ) );
  }
  return p->payload;
}

template <typename T>
bool Item::hasPayload() const
{
  BOOST_STATIC_ASSERT( !boost::is_pointer<T>::value );

  if ( !hasPayload() )
    return false;

  typedef Internal::PayloadTrait<T> PayloadType;
  if ( PayloadType::isPolymorphic ) {
    try {
      const typename PayloadType::SuperType sp = payload<typename PayloadType::SuperType>();
      return PayloadType::canCastFrom( sp );
    } catch ( const Akonadi::PayloadException& ) {}
  }

  return Internal::payload_cast<T>( payloadBase() );
}

template <typename T>
void Item::setPayload( const T &p )
{
  typedef Internal::PayloadTrait<T> PayloadType;
  if ( PayloadType::isPolymorphic ) {
    const typename PayloadType::SuperType sp
      = PayloadType::template castTo<typename PayloadType::SuperElementType>( p );
    if ( !Internal::PayloadTrait<typename PayloadType::SuperType>::isNull( sp )
         || PayloadType::isNull( p ) )
    {
      setPayload( sp );
      return;
    }
  }
  setPayloadBase( new Payload<T>( p ) );
}

template <typename T>
void Item::setPayload( T* p )
{
  p.You_MUST_NOT_use_a_pointer_as_payload;
}

template <typename T>
void Item::setPayload( std::auto_ptr<T> p )
{
  p.Nice_try_but_a_std_auto_ptr_is_not_allowed_as_payload_either;
}

}

Q_DECLARE_METATYPE(Akonadi::Item)
Q_DECLARE_METATYPE(Akonadi::Item::List)

#endif
