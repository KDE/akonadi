/*
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

#include "collectionlistjob.h"
#include "collectionpathresolver.h"

#include <klocale.h>

#include <QStringList>

using namespace Akonadi;

class CollectionPathResolver::Private
{
  public:
    int colId;
    QString path;
    bool pathToId;
    QStringList pathParts;
    Collection currentNode;
};

CollectionPathResolver::CollectionPathResolver(const QString & path, QObject * parent)
  : Job( parent ),
    d( new Private )
{
  d->pathToId = true;
  d->path = path;
  if ( d->path.startsWith( Collection::delimiter() )  )
    d->path = d->path.right( d->path.length() - Collection::delimiter().length() );
  if ( d->path.endsWith( Collection::delimiter() )  )
    d->path = d->path.left( d->path.length() - Collection::delimiter().length() );

  d->pathParts = d->path.split( Collection::delimiter() );
  d->currentNode = Collection::root();
}

CollectionPathResolver::CollectionPathResolver(const Collection & collection, QObject * parent)
  : Job( parent ),
    d( new Private )
{
  d->pathToId = false;
  d->colId = collection.id();
  d->currentNode = collection;
}

CollectionPathResolver::~CollectionPathResolver()
{
  delete d;
}

int CollectionPathResolver::collection() const
{
  return d->colId;
}

QString CollectionPathResolver::path() const
{
  if ( d->pathToId )
    return d->path;
  return d->pathParts.join( Collection::delimiter() );
}

void CollectionPathResolver::doStart()
{
  CollectionListJob *job = 0;
  if ( d->pathToId ) {
    if ( d->path.isEmpty() ) {
      d->colId = Collection::root().id();
      emitResult();
      return;
    }
    job = new CollectionListJob( d->currentNode, CollectionListJob::Flat, this );
  } else {
    if ( d->colId == 0 ) {
      d->colId = Collection::root().id();
      emitResult();
      return;
    }
    job = new CollectionListJob( d->currentNode, CollectionListJob::Local, this );
  }
  connect( job, SIGNAL(result(KJob*)), SLOT(jobResult(KJob*)) );
}

void CollectionPathResolver::jobResult(KJob *job )
{
  if ( job->error() )
    return;

  CollectionListJob *list = static_cast<CollectionListJob*>( job );
  CollectionListJob *nextJob = 0;
  const Collection::List cols = list->collections();
  if ( cols.isEmpty() ) {
      setError( Unknown );
      setErrorText( i18n( "No such collection.") );
      emitResult();
      return;
  }

  if ( d->pathToId ) {
    const QString currentPart = d->pathParts.takeFirst();
    bool found = false;
    foreach ( const Collection c, cols ) {
      if ( c.name() == currentPart ) {
        d->currentNode = c;
        found = true;
        break;
      }
    }
    if ( !found ) {
      setError( Unknown );
      setErrorText( i18n( "No such collection.") );
      emitResult();
      return;
    }
    if ( d->pathParts.isEmpty() ) {
      d->colId = d->currentNode.id();
      emitResult();
      return;
    }
    nextJob = new CollectionListJob( d->currentNode, CollectionListJob::Flat, this );
  } else {
    Collection col = list->collections().first();
    d->currentNode = Collection( col.parent() );
    d->pathParts.prepend( col.name() );
    if ( d->currentNode == Collection::root() ) {
      emitResult();
      return;
    }
    nextJob = new CollectionListJob( d->currentNode, CollectionListJob::Local, this );
  }
  connect( nextJob, SIGNAL(result(KJob*)), SLOT(jobResult(KJob*)) );
}

#include "collectionpathresolver.moc"
