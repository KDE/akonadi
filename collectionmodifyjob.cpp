/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "collectionmodifyjob.h"
#include "imapparser.h"

using namespace Akonadi;

class Akonadi::CollectionModifyJobPrivate
{
  public:
    Collection collection;
    QStringList mimeTypes;
    int policyId;
    bool setMimeTypes;
    bool setPolicy;
    QString name;
    Collection parent;
    QList<QPair<QByteArray,QByteArray> > attributes;
    QList<QByteArray> removeAttributes;
};

CollectionModifyJob::CollectionModifyJob(const Collection &collection, QObject * parent) :
    Job( parent ), d( new CollectionModifyJobPrivate )
{
  Q_ASSERT( collection.isValid() );
  d->collection = collection;
  d->setMimeTypes = false;
  d->setPolicy = false;
}

CollectionModifyJob::~ CollectionModifyJob()
{
  delete d;
}

void CollectionModifyJob::doStart()
{
  QByteArray command = newTag() + " MODIFY " + QByteArray::number( d->collection.id() );
  QByteArray changes;
  if ( d->setMimeTypes )
  {
    QList<QByteArray> bList;
    foreach( QString s, d->mimeTypes ) bList << s.toLatin1();
    changes += " MIMETYPE (" + ImapParser::join( bList, " " ) + ')';
  }
  if ( d->setPolicy )
    changes += " CACHEPOLICY " + QByteArray::number( d->policyId );
  if ( d->parent.isValid() )
    changes += " PARENT " + QByteArray::number( d->parent.id() );
  if ( !d->name.isEmpty() )
    changes += " NAME \"" + d->name.toUtf8() + '"';
  if ( !d->collection.remoteId().isNull() )
    changes += " REMOTEID \"" + d->collection.remoteId().toUtf8() + '"';
  typedef QPair<QByteArray,QByteArray> QByteArrayPair;
  foreach ( const QByteArrayPair bp, d->attributes )
    changes += ' ' + bp.first + ' ' + bp.second;
  foreach ( const QByteArray b, d->removeAttributes )
    changes += " -" + b;
  if ( changes.isEmpty() ) {
    emitResult();
    return;
  }
  command += changes + '\n';
  writeData( command );
}

void CollectionModifyJob::setContentTypes(const QStringList & mimeTypes)
{
  d->setMimeTypes = true;
  d->mimeTypes = mimeTypes;
}

void CollectionModifyJob::setCachePolicy(int policyId)
{
  d->policyId = policyId;
  d->setPolicy = true;
}

void CollectionModifyJob::setName(const QString & name)
{
  d->name = name;
}

void CollectionModifyJob::setParent(const Collection & parent)
{
  d->parent = parent;
}

void CollectionModifyJob::setAttribute(CollectionAttribute * attr)
{
  Q_ASSERT( !attr->type().isEmpty() );

  QByteArray value = ImapParser::quote( attr->toByteArray() );
  d->attributes.append( qMakePair( attr->type(), value ) );
}

void CollectionModifyJob::removeAttribute(const QByteArray & attrName)
{
  d->removeAttributes << attrName;
}

#include "collectionmodifyjob.moc"
