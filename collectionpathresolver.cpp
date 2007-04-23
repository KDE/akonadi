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

#include <QtCore/QStringList>

using namespace Akonadi;

class CollectionPathResolver::Private
{
  public:
    Private( CollectionPathResolver *parent )
      : mParent( parent )
    {
    }

    void jobResult( KJob* );

    CollectionPathResolver *mParent;
    int colId;
    QString path;
    bool pathToId;
    QStringList pathParts;
    Collection currentNode;
};

void CollectionPathResolver::Private::jobResult(KJob *job )
{
  if ( job->error() )
    return;

  CollectionListJob *list = static_cast<CollectionListJob*>( job );
  CollectionListJob *nextJob = 0;
  const Collection::List cols = list->collections();
  if ( cols.isEmpty() ) {
      mParent->setError( Unknown );
      mParent->setErrorText( i18n( "No such collection.") );
      mParent->emitResult();
      return;
  }

  if ( pathToId ) {
    const QString currentPart = pathParts.takeFirst();
    bool found = false;
    foreach ( const Collection c, cols ) {
      if ( c.name() == currentPart ) {
        currentNode = c;
        found = true;
        break;
      }
    }
    if ( !found ) {
      mParent->setError( Unknown );
      mParent->setErrorText( i18n( "No such collection.") );
      mParent->emitResult();
      return;
    }
    if ( pathParts.isEmpty() ) {
      colId = currentNode.id();
      mParent->emitResult();
      return;
    }
    nextJob = new CollectionListJob( currentNode, CollectionListJob::Flat, mParent );
  } else {
    Collection col = list->collections().first();
    currentNode = Collection( col.parent() );
    pathParts.prepend( col.name() );
    if ( currentNode == Collection::root() ) {
      mParent->emitResult();
      return;
    }
    nextJob = new CollectionListJob( currentNode, CollectionListJob::Local, mParent );
  }
  mParent->connect( nextJob, SIGNAL(result(KJob*)), mParent, SLOT(jobResult(KJob*)) );
}

CollectionPathResolver::CollectionPathResolver(const QString & path, QObject * parent)
  : Job( parent ),
    d( new Private( this ) )
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
    d( new Private( this ) )
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

#include "collectionpathresolver.moc"
