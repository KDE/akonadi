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

#include "itemserializer_p.h"
#include "item.h"
#include "itemserializerplugin.h"
#include "typepluginloader_p.h"

// Qt
#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtCore/QString>

namespace Akonadi {

DefaultItemSerializerPlugin::DefaultItemSerializerPlugin()
{
}

bool DefaultItemSerializerPlugin::deserialize( Item& item, const QByteArray& label, QIODevice& data, int )
{
  if ( label != Item::FullPayload )
    return false;

  item.setPayload( data.readAll() );
  return true;
}

void DefaultItemSerializerPlugin::serialize( const Item& item, const QByteArray& label, QIODevice& data, int& )
{
  Q_ASSERT( label == Item::FullPayload );
  Q_UNUSED( label );
  if ( item.hasPayload<QByteArray>() )
    data.write( item.payload<QByteArray>() );
}

/*static*/
void ItemSerializer::deserialize( Item& item, const QByteArray& label, const QByteArray& data, int version, bool external )
{
  if ( external ) {
    QFile file( QString::fromUtf8(data) );
    if ( file.open( QIODevice::ReadOnly ) ) {
      deserialize( item, label, file, version );
      file.close();
    }
  } else {
    QBuffer buffer;
    buffer.setData( data );
    buffer.open( QIODevice::ReadOnly );
    buffer.seek( 0 );
    deserialize( item, label, buffer, version );
    buffer.close();
  }
}

/*static*/
void ItemSerializer::deserialize( Item& item, const QByteArray& label, QIODevice& data, int version )
{
  if ( !TypePluginLoader::pluginForMimeType( item.mimeType() )->deserialize( item, label, data, version ) ) {
    kWarning() << "Unable to deserialize payload part:" << label;
    data.seek( 0 );
    kWarning() << "Payload data was: " << data.readAll();
  }
}

/*static*/
void ItemSerializer::serialize( const Item& item, const QByteArray& label, QByteArray& data, int &version )
{
  QBuffer buffer;
  buffer.setBuffer( &data );
  buffer.open( QIODevice::WriteOnly );
  buffer.seek( 0 );
  serialize( item, label, buffer, version );
  buffer.close();
}

/*static*/
void ItemSerializer::serialize( const Item& item, const QByteArray& label, QIODevice& data, int &version )
{
  if ( !item.hasPayload() )
    return;
  ItemSerializerPlugin *plugin = TypePluginLoader::pluginForMimeType( item.mimeType() );
  plugin->serialize( item, label, data, version );
}

void ItemSerializer::apply( Item &item, const Item &other )
{
  if ( !other.hasPayload() )
    return;

  ItemSerializerPlugin *plugin = TypePluginLoader::pluginForMimeType( item.mimeType() );

  ItemSerializerPluginV2 *pluginV2 = dynamic_cast<ItemSerializerPluginV2*>( plugin );
  if ( pluginV2 ) {
    pluginV2->apply( item, other );
    return;
  }

  // Old-school merge:
  foreach ( const QByteArray &part, other.loadedPayloadParts() ) {
    QByteArray partData;
    QBuffer buffer;
    buffer.setBuffer( &partData );
    buffer.open( QIODevice::ReadWrite );
    buffer.seek( 0 );
    int version;
    serialize( other, part, buffer, version );
    buffer.seek( 0 );
    deserialize( item, part, buffer, version );
  }
}

QSet<QByteArray> ItemSerializer::parts( const Item & item )
{
  if ( !item.hasPayload() )
    return QSet<QByteArray>();
  return TypePluginLoader::pluginForMimeType( item.mimeType() )->parts( item );
}

QSet<QByteArray> ItemSerializer::availableParts( const Item & item )
{
  if ( !item.hasPayload() )
    return QSet<QByteArray>();
  ItemSerializerPlugin *plugin = TypePluginLoader::pluginForMimeType( item.mimeType() );
  ItemSerializerPluginV2 *pluginV2 = dynamic_cast<ItemSerializerPluginV2*>( plugin );

  if ( pluginV2 )
    return pluginV2->availableParts( item );

  if (item.hasPayload())
    return QSet<QByteArray>();

  return QSet<QByteArray>() << Item::FullPayload;
}

}
