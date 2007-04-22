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
#include <QByteArray>
#include <QIODevice>
#include <QDebug>

namespace Akonadi {

class DefaultItemSerializerPlugin;

static DefaultItemSerializerPlugin * s_p = 0;

class ItemSerializerPlugin 
{
public:
    virtual ~ItemSerializerPlugin() { };
    virtual void deserialize( Item& item, const QString& label, const QByteArray& data ) const = 0;
    virtual void deserialize( Item& item, const QString& label, const QIODevice& data ) const = 0;
    virtual void serialize( const Item& item, const QString& label, QByteArray& data ) const = 0;
    virtual void serialize( const Item& item, const QString& label, QIODevice& data ) const = 0;
};

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
        item.setData( data );
    }

    void deserialize( Item& item, const QString& label, const QIODevice& data ) const
    {
        throw ItemSerializerException();
    }

    void serialize( const Item& item, const QString& label, QByteArray& data ) const
    {
        throw ItemSerializerException();
    }

    void serialize( const Item& item, const QString& label, QIODevice& data ) const
    {
        throw ItemSerializerException();
    }

};

}

using namespace Akonadi;

/*static*/ 
void ItemSerializer::deserialize( Item& item, const QString& label, const QByteArray& data )
{
    ItemSerializer::pluginForMimeType( item.mimeType() ).deserialize( item, label, data );
}

/*static*/ 
void ItemSerializer::deserialize( Item& item, const QString& label, const QIODevice& data )
{
    ItemSerializer::pluginForMimeType( item.mimeType() ).deserialize( item, label, data );
}

/*static*/ 
void ItemSerializer::serialize( const Item& item, const QString& label, QByteArray& data )
{
    ItemSerializer::pluginForMimeType( item.mimeType() ).serialize( item, label, data );
}

/*static*/ 
void ItemSerializer::serialize( const Item& item, const QString& label, QIODevice& data )
{
    ItemSerializer::pluginForMimeType( item.mimeType() ).serialize( item, label, data );
}

/*static*/ 
const ItemSerializerPlugin& ItemSerializer::pluginForMimeType( const QString & mimetype )
{
    ItemSerializerPlugin *plugin = DefaultItemSerializerPlugin::instance();
    
    // Go finding the right plugin for the mimetype
    //



    Q_ASSERT(plugin);
    return *plugin;
}


