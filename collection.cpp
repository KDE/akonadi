/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "collection.h"
#include "collectionattributefactory.h"

#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QStringList>

using namespace Akonadi;

class Collection::Private : public QSharedData
{
  public:
    Private() :
      QSharedData(),
      id( -1 ),
      parentId( -1 ),
      type( Unknown ),
      cachePolicyId( -1 )
    {}

    Private( const Private &other ) :
      QSharedData( other )
    {
      id = other.id;
      parentId = other.parentId;
      name = other.name;
      remoteId = other.remoteId;
      parentRemoteId = other.parentRemoteId;
      type = other.type;
      foreach ( CollectionAttribute* attr, other.attributes )
        attributes.insert( attr->type(), attr->clone() );
      rawAttributes = other.rawAttributes;
      resource = other.resource;
      cachePolicyId = other.cachePolicyId;
      status = other.status;
      contentTypes = other.contentTypes;
    }

    ~Private()
    {
      qDeleteAll( attributes );
    }

    int id;
    int parentId;
    QString name;
    QString remoteId;
    QString parentRemoteId;
    Type type;
    QHash<QByteArray, CollectionAttribute*> attributes;
    QHash<QByteArray, QByteArray> rawAttributes;
    QString resource;
    int cachePolicyId;
    CollectionStatus status;
    QStringList contentTypes;
};

Collection::Collection() :
    d ( new Private )
{
  static int lastId = -1;
  d->id = lastId--;
}

Collection::Collection( int id ) :
    d( new Private )
{
  d->id = id;
}

Collection::Collection(const Collection & other) :
    d ( other.d )
{
}

Collection::~Collection()
{
}

int Akonadi::Collection::id() const
{
  return d->id;
}

QString Collection::name( ) const
{
  return d->name;
}

void Collection::setName( const QString & name )
{
  d->name = name;
}

Collection::Type Collection::type() const
{
  return d->type;
}

void Collection::setType( Type type )
{
  d->type = type;
}

QStringList Collection::contentTypes() const
{
  return d->contentTypes;
}

void Collection::setContentTypes( const QStringList & types )
{
  d->contentTypes = types;
}

int Collection::parent() const
{
  return d->parentId;
}

void Collection::setParent( int parent )
{
  d->parentId = parent;
}

void Collection::setParent(const Collection & collection)
{
  d->parentId = collection.parent();
  d->parentRemoteId = collection.parentRemoteId();
}

QString Collection::parentRemoteId() const
{
  return d->parentRemoteId;
}

void Collection::setParentRemoteId(const QString & remoteParent)
{
  d->parentRemoteId = remoteParent;
}

QString Collection::delimiter()
{
  return QLatin1String( "/" );
}

Collection Collection::root( )
{
  Collection root( 0 );
  QStringList types;
  types << collectionMimeType();
  root.setContentTypes( types );
  return root;
}

QString Collection::collectionMimeType( )
{
  return QString::fromLatin1("inode/directory");
}

QList<CollectionAttribute*> Collection::attributes() const
{
  return d->attributes.values();
}

void Collection::addAttribute( CollectionAttribute * attr )
{
  if ( d->attributes.contains( attr->type() ) )
    delete d->attributes.take( attr->type() );
  d->attributes.insert( attr->type(), attr );
}

CollectionAttribute * Collection::attribute( const QByteArray & type ) const
{
  if ( d->attributes.contains( type ) )
    return d->attributes.value( type );
  return 0;
}

bool Collection::hasAttribute( const QByteArray & type ) const
{
  return d->attributes.contains( type );
}

bool Collection::isValid() const
{
  return id() >= 0;
}

bool Collection::operator ==(const Collection & other) const
{
  return d->id == other.id();
}

bool Collection::operator !=(const Collection & other) const
{
  return d->id != other.id();
}

QString Collection::remoteId() const
{
  return d->remoteId;
}

void Collection::setRemoteId(const QString & remoteId)
{
  d->remoteId = remoteId;
}

Collection& Collection::operator =(const Collection & other)
{
  if ( this != &other )
    d = other.d;

  return *this;
}

QString Collection::resource() const
{
  return d->resource;
}

void Collection::setResource(const QString & resource)
{
  d->resource = resource;
}

uint qHash( const Akonadi::Collection &collection )
{
  return qHash( collection.id() );
}

void Collection::addRawAttribute(const QByteArray & type, const QByteArray & value)
{
  CollectionAttribute* attr = CollectionAttributeFactory::createAttribute( type );
  if ( attr ) {
    attr->setData( value );
    addAttribute( attr );
  } else {
    d->rawAttributes.insert( type, value );
  }
}

int Collection::cachePolicyId() const
{
  return d->cachePolicyId;
}

void Collection::setCachePolicyId(int cachePolicyId)
{
  d->cachePolicyId = cachePolicyId;
}

CollectionStatus Collection::status() const
{
  return d->status;
}

void Collection::setStatus(const CollectionStatus & status)
{
  d->status = status;
}
