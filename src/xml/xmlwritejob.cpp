/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "xmlwritejob.h"
#include "xmldocument.h"
#include "xmlwriter.h"

#include "collection.h"
#include "collectionfetchjob.h"
#include "item.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"

#include <QDebug>

#include <QDomElement>
#include <QStack>

using namespace Akonadi;

namespace Akonadi {

class XmlWriteJobPrivate {
  public:
    XmlWriteJobPrivate( XmlWriteJob* parent ) : q( parent ) {}

    XmlWriteJob* const q;
    Collection::List roots;
    QStack<Collection::List> pendingSiblings;
    QStack<QDomElement> elementStack;
    QString fileName;
    XmlDocument document;

    void collectionFetchResult( KJob* job );
    void processCollection();
    void itemFetchResult( KJob* job );
    void processItems();
};

}

void XmlWriteJobPrivate::collectionFetchResult(KJob* job)
{
  if ( job->error() )
    return;
  CollectionFetchJob *fetch = dynamic_cast<CollectionFetchJob*>( job );
  Q_ASSERT( fetch );
  if ( fetch->collections().isEmpty() ) {
    processItems();
  } else {
    pendingSiblings.push( fetch->collections() );
    processCollection();
  }
}

void XmlWriteJobPrivate::processCollection()
{
  if ( !pendingSiblings.isEmpty() && pendingSiblings.top().isEmpty() ) {
    pendingSiblings.pop();
    if ( pendingSiblings.isEmpty() ) {
      q->done();
      return;
    }
    processItems();
    return;
  }

  if ( pendingSiblings.isEmpty() ) {
    q->done();
    return;
  }

  const Collection current = pendingSiblings.top().first();
  qDebug() << "Writing " << current.name() << "into" << elementStack.top().attribute( QStringLiteral("name") );
  elementStack.push( XmlWriter::writeCollection( current, elementStack.top() ) );
  CollectionFetchJob *subfetch = new CollectionFetchJob( current, CollectionFetchJob::FirstLevel, q );
  q->connect( subfetch, SIGNAL(result(KJob*)), q, SLOT(collectionFetchResult(KJob*)) );
}

void XmlWriteJobPrivate::processItems()
{
  const Collection collection = pendingSiblings.top().first();
  ItemFetchJob *fetch = new ItemFetchJob( collection, q );
  fetch->fetchScope().fetchAllAttributes();
  fetch->fetchScope().fetchFullPayload();
  q->connect( fetch, SIGNAL(result(KJob*)), q, SLOT(itemFetchResult(KJob*)) );
}

void XmlWriteJobPrivate::itemFetchResult(KJob* job)
{
  if ( job->error() )
    return;
  ItemFetchJob *fetch = dynamic_cast<ItemFetchJob*>( job );
  Q_ASSERT( fetch );
  foreach ( const Item &item, fetch->items() )
    XmlWriter::writeItem( item, elementStack.top() );
  pendingSiblings.top().removeFirst();
  elementStack.pop();
  processCollection();
}


XmlWriteJob::XmlWriteJob(const Collection& root, const QString& fileName, QObject* parent) :
  Job( parent ),
  d( new XmlWriteJobPrivate( this ) )
{
  d->roots.append( root );
  d->fileName = fileName;
}


XmlWriteJob::XmlWriteJob(const Collection::List& roots, const QString& fileName, QObject* parent) :
  Job( parent ),
  d( new XmlWriteJobPrivate( this ) )
{
  d->roots = roots;
  d->fileName = fileName;
}


XmlWriteJob::~XmlWriteJob()
{
  delete d;
}

void XmlWriteJob::doStart()
{
  d->elementStack.push( d->document.document().documentElement() );
  CollectionFetchJob *job = new CollectionFetchJob( d->roots, this );
  connect( job, SIGNAL(result(KJob*)), SLOT(collectionFetchResult(KJob*)) );
}

void XmlWriteJob::done() // cannot be in the private class due to emitResult()
{
  if ( !d->document.writeToFile( d->fileName ) ) {
    setError( Unknown );
    setErrorText( d->document.lastError() );
  }
  emitResult();
}

#include "moc_xmlwritejob.cpp"
