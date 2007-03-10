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
    QList<QByteArray> mimeTypes;
    int policyId;
    bool setMimeTypes;
    bool setPolicy;
    QString name;
    Collection parent;
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
  QByteArray command = newTag() + " MODIFY " + QByteArray::number( d->collection.id() ) + ' ';
  if ( d->setMimeTypes )
    command += " MIMETYPE (" + ImapParser::join( d->mimeTypes, " " ) + ')';
  if ( d->setPolicy )
    command += " CACHEPOLICY " + QByteArray::number( d->policyId );
  if ( d->parent.isValid() )
    command += " PARENT " + QByteArray::number( d->parent.id() );
  if ( !d->name.isEmpty() )
    command += " NAME \"" + d->name.toUtf8() + '"';
  writeData( command );
}

void CollectionModifyJob::setContentTypes(const QList< QByteArray > & mimeTypes)
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

#include "collectionmodifyjob.moc"
