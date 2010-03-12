/*
  Copyright (C) 2009 Stephen Kelly <steveire@gmail.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "fakeserverdata.h"
#include "fakeserver.h"

#include <QLocalServer>
#include <KStandardDirs>
#include <QDir>
#include <Akonadi/ItemFetchJob>

FakeServerData::FakeServerData(PublicETM *model, FakeSession *session, FakeMonitor *monitor, QObject *parent)
  : QObject(parent),
    m_model( model ),
    m_session( session ),
    m_monitor( monitor ),
    m_jobsActioned( 0 ),
    m_nextCollectionId( 1 ),
    m_nextItemId( 0 )
{
  connect(session, SIGNAL(jobAdded(Akonadi::Job *)), SLOT(jobAdded(Akonadi::Job *)), Qt::QueuedConnection);
  connect(this, SIGNAL(emit_itemsFetched(Akonadi::Item::List)), model, SLOT(itemsFetched(Akonadi::Item::List)));
}

QList<FakeServerData::Token> FakeServerData::tokenize(const QString& treeString)
{
  QStringList parts = treeString.split("-");

  QList<Token> tokens;
  const QStringList::const_iterator begin = parts.constBegin();
  const QStringList::const_iterator end = parts.constEnd();

  QStringList::const_iterator it = begin;
  ++it;
  for (; it != end; ++it)
  {
    Token token;
    if (it->trimmed().isEmpty())
    {
      token.type = Token::Branch;
    } else {
      token.type = Token::Leaf;
      token.content = it->trimmed();
    }
    tokens.append(token);
  }
  return tokens;
}

void FakeServerData::interpret( const QString& serverData )
{
  m_serverDataString = serverData;

  parseTreeString(m_serverDataString);


}

void FakeServerData::parseTreeString(const QString& treeString)
{
  int depth = 0;

  m_recentCollections.append( Collection::root() );

  QList<Token> tokens = tokenize( treeString );
  while(!tokens.isEmpty())
  {
    Token token = tokens.takeFirst();

    if (token.type == Token::Branch)
    {
      ++depth;
      continue;
    }
    Q_ASSERT(token.type == Token::Leaf);
    parseEntityString( token.content, depth );

    depth = 0;
  }
}

void FakeServerData::parseEntityString(const QString& _entityString, int depth )
{
  QString entityString = _entityString;
  if ( entityString.startsWith( "C" ) )
  {
    Collection collection;
    entityString.remove( 0, 2 );
    Q_ASSERT( entityString.startsWith( '(' ) );
    entityString.remove( 0, 1 );
    QStringList parts = entityString.split( ')' );

    if ( !parts.first().isEmpty() )
    {
      QString typesString = parts.takeFirst();

      QStringList types = typesString.split(',');
      types.replaceInStrings(" ", "");
      collection.setContentMimeTypes( types );
    } else {
      parts.removeFirst();
    }

    collection.setId( m_nextCollectionId++ );
    collection.setName( QString("Collection %1").arg( collection.id() ) );
    collection.setRemoteId( QString( "remoteId %1" ).arg( collection.id() ) );

    if ( depth == 0 )
      collection.setParentCollection( Collection::root() );
    else
    {
      collection.setParentCollection( m_recentCollections.at( depth - 1 ) );
    }

    m_collections.insert( collection.id(), collection );

    if ( m_recentCollections.size() == depth )
      m_recentCollections.append( collection );
    else
      m_recentCollections[ depth ] = collection;

    int order = 0;
    if ( !parts.first().isEmpty() )
    {
      QString orderString = parts.first().trimmed();
      if (!orderString.isEmpty())
      {
        bool ok;
        order = orderString.toInt(&ok);
        Q_ASSERT(ok);
      }
    } else
    {
      order = 1;
    }
    while ( m_collectionSequence.size() < order )
    {
      m_collectionSequence.append( Collection::List() );
    }
    m_collectionSequence[ order - 1 ].append( collection );
  }
  if ( entityString.startsWith( "I" ) )
  {
    Item item;
    entityString.remove( 0, 2 );
    QString type;
    int order = 0;
    if ( entityString.contains( ' ' ) )
    {
      QStringList parts = entityString.split( ' ' );
      type = parts.takeFirst();
      bool ok;
      order = parts.first().toInt(&ok);
    } else
      type = entityString;
    item.setMimeType( type );
    item.setId( m_nextItemId++ );
    item.setRemoteId( QString( "RId_%1 %2" ).arg( item.id() ).arg(type) );
    item.setParentCollection( m_recentCollections.at( depth - 1 ) );

    m_items.insert( item.id(), item );
    m_childElements[ item.parentCollection().id() ].append( item.id() );
  }
}

void FakeServerData::jobAdded( Job* job )
{
  if ( m_jobsActioned == 0 )
  {
    CollectionFetchJob *colFetch = qobject_cast<CollectionFetchJob*>( job );

    QListIterator<Collection::List> it(m_collectionSequence);

    it.toBack();
    while ( it.hasPrevious())
    {
      Collection::List list = it.previous();
      m_model->privateClass()->collectionsFetched( list );
    }

    ++m_jobsActioned;
  } else {
    ItemFetchJob *itemFetch = qobject_cast<ItemFetchJob*>( job );
    Item::List list;
    qint64 fetchColId = job->property( "ItemFetchCollectionId" ).toULongLong();
    foreach (Entity::Id id, m_childElements.value( fetchColId ) )
      list << m_items.value( id );

    setProperty( "ItemFetchCollectionId", fetchColId );

    emit_itemsFetched( list );
  }
}

#include "fakeserverdata.moc"
