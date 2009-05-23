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
#include "attributefactory.h"

// KDE core
#include <kdebug.h>
#include <kmimetype.h>
#include <kglobal.h>

// Qt
#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QStringList>

// temporary
#include "pluginloader_p.h"



namespace Akonadi {

class DefaultItemSerializerPlugin;

class DefaultItemSerializerPlugin : public ItemSerializerPlugin
{
public:
    DefaultItemSerializerPlugin() { }

    bool deserialize( Item& item, const QByteArray& label, QIODevice& data, int )
    {
        if ( label != Item::FullPayload )
          return false;
        item.setPayload( data.readAll() );
        return true;
    }

    void serialize( const Item& item, const QByteArray& label, QIODevice& data, int& )
    {
        Q_ASSERT( label == Item::FullPayload );
        if ( item.hasPayload<QByteArray>() )
            data.write( item.payload<QByteArray>() );
    }

};

K_GLOBAL_STATIC( DefaultItemSerializerPlugin, s_defaultItemSerializerPlugin )

}


using namespace Akonadi;

class PluginEntry
{
  public:
    PluginEntry()
      : mPlugin( 0 )
    {
    }

    PluginEntry( const QString &identifier )
      : mIdentifier( identifier ), mPlugin( 0 )
    {
    }

    PluginEntry( ItemSerializerPlugin* plugin ) : mPlugin( plugin ) {}

    inline ItemSerializerPlugin* plugin() const
    {
      if ( mPlugin )
        return mPlugin;

      QObject *object = PluginLoader::self()->createForName( mIdentifier );
      if ( !object ) {
        kWarning( 5250 ) << "ItemSerializerPluginLoader: "
                         << "plugin" << mIdentifier << "is not valid!" << endl;

        // we try to use the default in that case
        mPlugin = s_defaultItemSerializerPlugin;
      }

      mPlugin = qobject_cast<ItemSerializerPlugin*>( object );
      if ( !mPlugin ) {
        kWarning( 5250 ) << "ItemSerializerPluginLoader: "
                         << "plugin" << mIdentifier << "doesn't provide interface ItemSerializerPlugin!" << endl;

        // we try to use the default in that case
        mPlugin = s_defaultItemSerializerPlugin;
      }

      Q_ASSERT( mPlugin );

      return mPlugin;
    }

  private:
    QString mIdentifier;
    mutable ItemSerializerPlugin *mPlugin;
};

static QHash<QString, PluginEntry> * all = 0;

static void loadPlugins() {
  const PluginLoader* pl = PluginLoader::self();
  if ( !pl ) {
    kWarning( 5250 ) << "Cannot instantiate plugin loader!" << endl;
    return;
  }
  const QStringList types = pl->types();
  kDebug( 5250 ) << "ItemSerializerPluginLoader: "
                 << "found" << types.size() << "plugins." << endl;
// FIXME: when adding this it might be found before more specific plugins if there is no exact match
//  all->insert( QLatin1String( "application/octet-stream" ), PluginEntry( s_defaultItemSerializerPlugin ) );
  for ( QStringList::const_iterator it = types.begin() ; it != types.end() ; ++it ) {
    all->insert( *it, PluginEntry( *it ) );
  }
}

static void setup()
{
    if (!all) {
        all = new QHash<QString, PluginEntry>();
        loadPlugins();
    }
}

/*static*/
void ItemSerializer::deserialize( Item& item, const QByteArray& label, const QByteArray& data, int version, bool external )
{

    if ( external ) {
      QFile file( QString::fromUtf8(data) );
      if ( file.open( QIODevice:: ReadOnly ) ) {
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
  setup();
  if ( !ItemSerializer::pluginForMimeType( item.mimeType() ).deserialize( item, label, data, version ) )
    kWarning() << "Unable to deserialize payload part:" << label;
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
  setup();
  ItemSerializerPlugin& plugin = pluginForMimeType( item.mimeType() );
  plugin.serialize( item, label, data, version );
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
        return *(all->value( mimetype ).plugin());

    KMimeType::Ptr mimeType = KMimeType::mimeType( mimetype, KMimeType::ResolveAliases );
    if ( !mimeType.isNull() ) {
      foreach ( const QString &type, all->keys() ) {
        if ( mimeType->is( type ) ) {
          return *(all->value( type ).plugin() );
        }
      }
    }

    kDebug( 5250 ) << "No plugin for mimetype " << mimetype << " found!";
    kDebug( 5250 ) << "Available plugins are: " << all->keys();

    ItemSerializerPlugin *plugin = s_defaultItemSerializerPlugin;
    Q_ASSERT(plugin);
    return *plugin;
}
