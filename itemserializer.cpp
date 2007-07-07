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

// Qt
#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtCore/QDebug>
#include <QtCore/QIODevice>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>

// libkdepim
#include <libkdepim/pluginloader.h>



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

    void deserialize( Item& item, const QString& label, QIODevice& data )
    {
        Q_ASSERT( label == Item::PartBody );
        item.setPayload( data.readAll() );
    }

    void serialize( const Item& item, const QString& label, QIODevice& data )
    {
        Q_ASSERT( label == Item::PartBody );
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

static QMap<QString, ItemSerializerPlugin*> * all = 0;

static void loadPlugins() {
  const ItemSerializerPluginLoader* pl = ItemSerializerPluginLoader::instance();
  if ( !pl ) {
    qWarning() << "ItemSerializerPluginLoader: cannot instantiate plugin loader!" << endl;
    return;
  }
  const QStringList types = pl->types();
  qDebug() << "ItemSerializerPluginLoader: found " << types.size() << " plugins." << endl;
  for ( QStringList::const_iterator it = types.begin() ; it != types.end() ; ++it ) {
    ItemSerializerPlugin * plugin = pl->createForName( *it );
    if ( !plugin ) {
      qWarning() << "ItemSerializerPlugin: plugin \"" << *it << "\" is not valid!" << endl;
      continue;
    }
    QStringList supportedTypes = (*it).split( QLatin1Char(',') );
    foreach ( const QString t, supportedTypes ) {
      qDebug() << "ItemSerializerPluginLoader: inserting plugin for type: " << t;
      all->insert( t, plugin );
    }
  }

  if ( !all->contains( QLatin1String("application/octed-stream") ) )
    all->insert( QLatin1String("application/octet-stream"), DefaultItemSerializerPlugin::instance() );
}

static void setup()
{
    if (!all) {
        all = new QMap<QString, ItemSerializerPlugin*>();
        loadPlugins();
    }
}

/*static*/
void ItemSerializer::deserialize( Item& item, const QString& label, const QByteArray& data )
{
    QBuffer buffer;
    buffer.setData( data );
    buffer.open( QIODevice::ReadOnly );
    buffer.seek( 0 );
    deserialize( item, label, buffer );
    buffer.close();
}

/*static*/
void ItemSerializer::deserialize( Item& item, const QString& label, QIODevice& data )
{
    setup();
    QStringList supportedParts = pluginForMimeType( item.mimeType() ).parts( item );
    if ( supportedParts.contains( label ) )
      ItemSerializer::pluginForMimeType( item.mimeType() ).deserialize( item, label, data );
    else
      item.addPart( label, data.readAll() );
}

/*static*/
void ItemSerializer::serialize( const Item& item, const QString& label, QByteArray& data )
{
    QBuffer buffer;
    buffer.setBuffer( &data );
    buffer.open( QIODevice::WriteOnly );
    buffer.seek( 0 );
    serialize( item, label, buffer );
    buffer.close();
}

/*static*/
void ItemSerializer::serialize( const Item& item, const QString& label, QIODevice& data )
{
    setup();
    QStringList supportedParts = pluginForMimeType( item.mimeType() ).parts( item );
    if ( !supportedParts.contains( label ) ) {
      data.write( item.part( label ) );
      return;
    }
    if ( !item.hasPayload() )
      return;
    ItemSerializer::pluginForMimeType( item.mimeType() ).serialize( item, label, data );
}

/*static*/
ItemSerializerPlugin& ItemSerializer::pluginForMimeType( const QString & mimetype )
{
    if ( all->contains( mimetype ) )
        return *(all->value(mimetype));

    qDebug() << "ItemSerializer: No plugin for mimetype " << mimetype << " found!";
    qDebug() << "available plugins are: " << all->keys();

    ItemSerializerPlugin *plugin = DefaultItemSerializerPlugin::instance();
    Q_ASSERT(plugin);
    return *plugin;
}
