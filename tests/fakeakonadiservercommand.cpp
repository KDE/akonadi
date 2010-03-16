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

#include "fakeakonadiservercommand.h"

#include <QStringList>
#include "fakeserverdata.h"
#include <akonadi/entitydisplayattribute.h>

using namespace Akonadi;

void FakeJobResponse::doCommand()
{
}

QList<FakeJobResponse::Token> FakeJobResponse::tokenize(const QString& treeString)
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

QList<FakeAkonadiServerCommand *> FakeJobResponse::interpret( FakeServerData *fakeServerData, const QString& serverData )
{
  QList<FakeAkonadiServerCommand *> list;
  QList<FakeJobResponse *> response = parseTreeString( fakeServerData, serverData );

  foreach( FakeJobResponse *command, response )
    list.append( command );
  return list;
}

QList<FakeJobResponse *> FakeJobResponse::parseTreeString( FakeServerData *fakeServerData, const QString& treeString)
{
  int depth = 0;

  QList<FakeJobResponse *> collectionResponseList;
  QHash<Collection::Id, FakeJobResponse *> itemResponseMap;

  Collection::List recentCollections;

  recentCollections.append( Collection::root() );

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
    parseEntityString( collectionResponseList, itemResponseMap, recentCollections, fakeServerData, token.content, depth );

    depth = 0;
  }
  return collectionResponseList;
}

void FakeJobResponse::parseEntityString( QList<FakeJobResponse *> &collectionResponseList, QHash<Collection::Id, FakeJobResponse *> &itemResponseMap, Collection::List &recentCollections, FakeServerData *fakeServerData, const QString& _entityString, int depth )
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

    collection.setId( fakeServerData->nextCollectionId() );
    collection.setName( QString("Collection %1").arg( collection.id() ) );
    collection.setRemoteId( QString( "remoteId %1" ).arg( collection.id() ) );

    if ( depth == 0 )
      collection.setParentCollection( Collection::root() );
    else
      collection.setParentCollection( recentCollections.at( depth ) );

    if ( recentCollections.size() == ( depth + 1) )
      recentCollections.append( collection );
    else
      recentCollections[ depth + 1 ] = collection;

    int order = 0;
    if ( !parts.first().isEmpty() )
    {
      QString displayName;
      QString optionalSection = parts.first().trimmed();
      if ( optionalSection.startsWith( QLatin1Char( '\'' ) ) )
      {
        optionalSection.remove( 0, 1 );
        QStringList optionalParts = optionalSection.split( QLatin1Char( '\'' ) );
        displayName = optionalParts.takeFirst();
        EntityDisplayAttribute *eda = new EntityDisplayAttribute();
        eda->setDisplayName( displayName );
        collection.addAttribute( eda );
        optionalSection = optionalParts.first();
      }

      QString orderString = optionalSection.trimmed();
      if ( !orderString.isEmpty() )
      {
        bool ok;
        order = orderString.toInt(&ok);
        Q_ASSERT(ok);
      }
    } else
    {
      order = 1;
    }
    while ( collectionResponseList.size() < order )
    {
      collectionResponseList.append( new FakeJobResponse( recentCollections[ depth ], FakeJobResponse::RespondToCollectionFetch ) );
    }
    collectionResponseList[ order - 1 ]->appendCollection( collection );
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
    item.setId( fakeServerData->nextItemId() );
    item.setRemoteId( QString( "RId_%1 %2" ).arg( item.id() ).arg(type) );
    item.setParentCollection( recentCollections.at( depth ) );

    Collection::Id colId = recentCollections[ depth ].id();
    if ( !itemResponseMap.contains( colId ) )
    {
      FakeJobResponse *newResponse = new FakeJobResponse( recentCollections[ depth ], FakeJobResponse::RespondToItemFetch );
      itemResponseMap.insert( colId, newResponse );
      collectionResponseList.append( newResponse );
    }
    itemResponseMap[ colId ]->appendItem( item );
  }
}
