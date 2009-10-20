/*
    Copyright (c) 2007 Till Adam <adam@kde.org>
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_ITEMSERIALIZERPLUGIN_H
#define AKONADI_ITEMSERIALIZERPLUGIN_H

#include <QtCore/QByteArray>
#include <QtCore/QSet>

#include "item.h"
#include "akonadi_export.h"

class QIODevice;

namespace Akonadi {


/**
 * @short The base class for item type serializer plugins.
 *
 * Serializer plugins convert between the payload of Akonadi::Item objects and
 * a textual or binary representation of the actual content data.
 * This allows to easily add support for new types to Akonadi.
 *
 * The following example shows how to implement a serializer plugin for
 * a new data type PimNote.
 *
 * The PimNote data structure:
 * @code
 * typedef struct {
 *   QString author;
 *   QDateTime dateTime;
 *   QString text;
 * } PimNote;
 * @endcode
 *
 * The serializer plugin code:
 * @code
 * #include <QtCore/qplugin.h>
 *
 * class SerializerPluginPimNote : public QObject, public Akonadi::ItemSerializerPlugin
 * {
 *   Q_OBJECT
 *   Q_INTERFACES( Akonadi::ItemSerializerPlugin )
 *
 *   public:
 *     bool deserialize( Akonadi::Item& item, const QByteArray& label, QIODevice& data, int version )
 *     {
 *       // we don't handle versions in this example
 *       Q_UNUSED( version );
 *
 *       // we work only on full payload
 *       if ( label != Akonadi::Item::FullPayload )
 *         return false;
 *
 *       QDataStream stream( &data );
 *
 *       PimNote note;
 *       stream >> note.author;
 *       stream >> note.dateTime;
 *       stream >> note.text;
 *
 *       item.setPayload<PimNote>( note );
 *
 *       return true;
 *     }
 *
 *     void serialize( const Akonadi::Item& item, const QByteArray& label, QIODevice& data, int &version )
 *     {
 *       // we don't handle versions in this example
 *       Q_UNUSED( version );
 *
 *       if ( label != Akonadi::Item::FullPayload || !item.hasPayload<PimNote>() )
 *         return;
 *
 *       QDataStream stream( &data );
 *
 *       PimNote note = item.payload<PimNote>();
 *
 *       stream << note.author;
 *       stream << note.dateTime;
 *       stream << note.text;
 *     }
 * };
 *
 * Q_EXPORT_PLUGIN2( akonadi_serializer_pimnote, SerializerPluginPimNote )
 *
 * @endcode
 *
 * The desktop file:
 * @code
 * [Misc]
 * Name=Pim Note Serializer
 * Comment=An Akonadi serializer plugin for note objects
 *
 * [Plugin]
 * Type=application/x-pimnote
 * X-KDE-Library=akonadi_serializer_pimnote
 * @endcode
 *
 * @author Till Adam <adam@kde.org>, Volker Krause <vkrause@kde.org>
 */
class AKONADI_EXPORT ItemSerializerPlugin
{
  public:
    /**
     * Destroys the item serializer plugin.
     */
    virtual ~ItemSerializerPlugin();

    /**
     * Converts serialized item data provided in @p data into payload for @p item.
     *
     * @param item The item to which the payload should be added.
     *             It is guaranteed to have a mime type matching one of the supported
     *             mime types of this plugin.
     *             However it might contain a unsuited payload added manually
     *             by the application developer.
     *             Verifying the payload type in case a payload is already available
     *             is recommended therefore.
     * @param label The part identifier of the part to deserialize.
     *              @p label might be an unsupported item part, return @c false if this is the case.
     * @param data A QIODevice providing access to the serialized data.
     *             The QIODevice is opened in read-only mode and positioned at the beginning.
     *             The QIODevice is guaranteed to be valid.
     * @param version The version of the data format as set by the user in serialize() or @c 0 (default).
     * @return @c false if the specified part is not supported by this plugin, @c true if the part
     *            could be de-serialized successfully.
     */
    virtual bool deserialize( Item& item, const QByteArray& label, QIODevice& data, int version ) = 0;

    /**
     * Convert the payload object provided in @p item into its serialzed form into @p data.
     *
     * @param item The item which contains the payload.
     *             It is guaranteed to have a mimetype matching one of the supported
     *             mimetypes of this plugin as well as the existence of a payload object.
     *             However it might contain an unsupported payload added manually by
     *             the application developer.
     *             Verifying the payload type is recommended therefore.
     * @param label The part identifier of the part to serialize.
     *              @p label will be one of the item parts returned by parts().
     * @param data The QIODevice where the serialized data should be written to.
     *             The QIODevice is opened in write-only mode and positioned at the beginning.
     *             The QIODevice is guaranteed to be valid.
     * @param version The version of the data format. Can be set by the user to handle different
     *                versions.
     */
    virtual void serialize( const Item& item, const QByteArray& label, QIODevice& data, int &version ) = 0;

    /**
     * Returns a list of available parts for the given item payload.
     * The default implementation returns Item::FullPayload if a payload is set.
     *
     * @param item The item.
     */
    virtual QSet<QByteArray> parts( const Item &item ) const;

};

/**
 * @short The extended base class for item type serializer plugins.
 *
 * @since 4.4
 */
class AKONADI_EXPORT ItemSerializerPluginV2 : public ItemSerializerPlugin
{
  public:
    /**
     * Destroys the item serializer plugin.
     */
    virtual ~ItemSerializerPluginV2();

    /**
     * Merges the payload parts in @p other into @p item.
     *
     * The default implementation is slow as it requires serializing @p other, and deserializing @p item multiple times.
     * Reimplementing this is recommended if your type uses payload parts.
     *
     * @since 4.4
     */
    virtual void apply( Item &item, const Item &other );

    /**
     * Returns the parts available in the item @p item.
     *
     * This should be reimplemented to return available parts.
     *
     * The default implementation returns an empty set if the item has a payload,
     * and a set containing Item::FullPayload if the item has no payload.
     *
     * @since 4.4
     */
    virtual QSet<QByteArray> availableParts( const Item &item ) const;
};

}

Q_DECLARE_INTERFACE( Akonadi::ItemSerializerPlugin, "org.freedesktop.Akonadi.ItemSerializerPlugin/1.0" )
Q_DECLARE_INTERFACE( Akonadi::ItemSerializerPluginV2, "org.freedesktop.Akonadi.ItemSerializerPlugin/1.1" )

#endif
