/*
    Copyright (c) 2007 Till Adam <adam@kde.org>

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

// Qt
#include <QByteArray>
#include <QIODevice>
#include <QDebug>
#include <QMap>
#include <QString>

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

    void deserialize( Item& item, const QString& label, const QByteArray& data ) const
    {
        item.setPayload( data );
    }

    void deserialize( Item& item, const QString& label, const QIODevice& data ) const
    {
        throw ItemSerializerException();
    }

    void serialize( const Item& item, const QString& label, QByteArray& data ) const
    {
        data = item.payload<QByteArray>();
    }

    void serialize( const Item& item, const QString& label, QIODevice& data ) const
    {
        throw ItemSerializerException();
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

static QMap<QString, const ItemSerializerPlugin*> * all = 0;

static void loadPlugins() {
  const ItemSerializerPluginLoader* pl = ItemSerializerPluginLoader::instance();
  if ( !pl ) {
    qWarning() << "ItemSerializerPluginLoader: cannot instantiate plugin loader!" << endl;
    return;
  }
  const QStringList types = pl->types();
  qDebug() << "ItemSerializerPluginLoader: found " << types.size() << " plugins." << endl;
  for ( QStringList::const_iterator it = types.begin() ; it != types.end() ; ++it ) {
    const ItemSerializerPlugin * plugin = pl->createForName( *it );
    if ( !plugin ) {
      qWarning() << "ItemSerializerPlugin: plugin \"" << *it << "\" is not valid!" << endl;
      continue;
    }
    qDebug() << "ItemSerializerPluginLoader: inserting plugin for type: " << *it;
    all->insert( *it, plugin );
  }
}

static void setup()
{
    if (!all) {
        all = new QMap<QString, const ItemSerializerPlugin*>();
        loadPlugins();
    }
}

/*static*/
void ItemSerializer::deserialize( Item& item, const QString& label, const QByteArray& data )
{
    setup();
    ItemSerializer::pluginForMimeType( item.mimeType() ).deserialize( item, label, data );
}

/*static*/
void ItemSerializer::deserialize( Item& item, const QString& label, const QIODevice& data )
{
    setup();
    ItemSerializer::pluginForMimeType( item.mimeType() ).deserialize( item, label, data );
}

/*static*/
void ItemSerializer::serialize( const Item& item, const QString& label, QByteArray& data )
{
    setup();
    ItemSerializer::pluginForMimeType( item.mimeType() ).serialize( item, label, data );
}

/*static*/
void ItemSerializer::serialize( const Item& item, const QString& label, QIODevice& data )
{
    setup();
    ItemSerializer::pluginForMimeType( item.mimeType() ).serialize( item, label, data );
}

/*static*/
const ItemSerializerPlugin& ItemSerializer::pluginForMimeType( const QString & mimetype )
{
    const ItemSerializerPlugin *plugin = DefaultItemSerializerPlugin::instance();

    // Go finding the right plugin for the mimetype
    qDebug() << "ItemSerializer: looking for plugin for mimetype " << mimetype;

    if ( all->contains( mimetype ) ) {
        qDebug() << "ItemSerializer: found plugin!";
        return *(all->value(mimetype));
    }

    Q_ASSERT(plugin);
    return *plugin;
}


