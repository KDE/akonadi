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

#include "itemserializer.h"
#include "item.h"
#include "itemserializerplugin.h"
#include "attributefactory.h"

// KDE core
#include <kdebug.h>
#include <kmimetype.h>

// Qt
#include <QtCore/QBuffer>
#include <QtCore/QIODevice>
#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QStringList>

// temporary
#include "pluginloader.h"



namespace Akonadi {

class DefaultItemSerializerPlugin;

static DefaultItemSerializerPlugin * s_p = 0;

class DefaultItemSerializerPlugin : public ItemSerializerPlugin
{
private:
    DefaultItemSerializerPlugin() { }
public:
    static DefaultItemSerializerPlugin * instance()
    {
        if ( !s_p ) {
            s_p = new DefaultItemSerializerPlugin;
        }
        return s_p;
    }

    bool deserialize( Item& item, const QByteArray& label, QIODevice& data )
    {
        if ( label != Item::FullPayload )
          return false;
        item.setPayload( data.readAll() );
        return true;
    }

    void serialize( const Item& item, const QByteArray& label, QIODevice& data )
    {
        Q_ASSERT( label == Item::FullPayload );
        if ( item.hasPayload<QByteArray>() )
            data.write( item.payload<QByteArray>() );
    }

};


namespace {

  KPIM_DEFINE_PLUGIN_LOADER( ItemSerializerPluginLoader,
                             Akonadi::ItemSerializerPlugin,
                             "create_item_serializer_plugin",
                             "akonadi/plugins/serializer/*.desktop" )

}


}

using namespace Akonadi;

static QHash<QString, ItemSerializerPlugin*> * all = 0;

static void loadPlugins() {
  const ItemSerializerPluginLoader* pl = ItemSerializerPluginLoader::instance();
  if ( !pl ) {
    kWarning( 5250 ) << "Cannot instantiate plugin loader!" << endl;
    return;
  }
  const QStringList types = pl->types();
  kDebug( 5250 ) << "ItemSerializerPluginLoader: "
                 << "found" << types.size() << "plugins." << endl;
  for ( QStringList::const_iterator it = types.begin() ; it != types.end() ; ++it ) {
    ItemSerializerPlugin * plugin = pl->createForName( *it );
    if ( !plugin ) {
      kWarning( 5250 ) << "ItemSerializerPluginLoader: "
                       << "plugin" << *it << "is not valid!" << endl;
      continue;
    }

    QStringList supportedTypes = (*it).split( QLatin1Char(',') );
    foreach ( const QString t, supportedTypes ) {
      kDebug( 5250 ) << "ItemSerializerPluginLoader: "
                     << "inserting plugin for type:" << t;
      all->insert( t, plugin );
    }
  }
}

static void setup()
{
    if (!all) {
        all = new QHash<QString, ItemSerializerPlugin*>();
        loadPlugins();
    }
}

/*static*/
void ItemSerializer::deserialize( Item& item, const QByteArray& label, const QByteArray& data )
{
    QBuffer buffer;
    buffer.setData( data );
    buffer.open( QIODevice::ReadOnly );
    buffer.seek( 0 );
    deserialize( item, label, buffer );
    buffer.close();
}

/*static*/
void ItemSerializer::deserialize( Item& item, const QByteArray& label, QIODevice& data )
{
    setup();
    if ( !ItemSerializer::pluginForMimeType( item.mimeType() ).deserialize( item, label, data ) ) {
      data.seek( 0 );
      Attribute* attr = AttributeFactory::createAttribute( label );
      Q_ASSERT( attr );
      attr->deserialize( data.readAll() );
      item.addAttribute( attr );
    }
}

/*static*/
void ItemSerializer::serialize( const Item& item, const QByteArray& label, QByteArray& data )
{
    QBuffer buffer;
    buffer.setBuffer( &data );
    buffer.open( QIODevice::WriteOnly );
    buffer.seek( 0 );
    serialize( item, label, buffer );
    buffer.close();
}

/*static*/
void ItemSerializer::serialize( const Item& item, const QByteArray& label, QIODevice& data )
{
    setup();
    ItemSerializerPlugin& plugin = pluginForMimeType( item.mimeType() );
    const QSet<QByteArray> supportedParts = plugin.parts( item );
    if ( !supportedParts.contains( label ) ) {
      Attribute* attr = item.attribute( label );
      if ( attr )
        data.write( attr->serialized() );
      return;
    }
    if ( !item.hasPayload() )
      return;
    plugin.serialize( item, label, data );
}

QSet<QByteArray> ItemSerializer::parts(const Item & item)
{
  if ( !item.hasPayload() )
    return QSet<QByteArray>();
  setup();
  return pluginForMimeType( item.mimeType() ).parts( item );
}

/*static*/
ItemSerializerPlugin& ItemSerializer::pluginForMimeType( const QString & mimetype )
{
    if ( all->contains( mimetype ) )
        return *(all->value(mimetype));

    KMimeType::Ptr mimeType = KMimeType::mimeType( mimetype, KMimeType::ResolveAliases );
    if ( !mimeType.isNull() ) {
      foreach ( const QString type, all->keys() ) {
        if ( mimeType->is( type ) ) {
          return *(all->value( type ) );
        }
      }
    }

    kDebug( 5250 ) << "No plugin for mimetype " << mimetype << " found!";
    kDebug( 5250 ) << "Available plugins are: " << all->keys();

    ItemSerializerPlugin *plugin = DefaultItemSerializerPlugin::instance();
    Q_ASSERT(plugin);
    return *plugin;
}
