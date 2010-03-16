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

#ifndef FAKE_AKONADI_SERVER_COMMAND_H
#define FAKE_AKONADI_SERVER_COMMAND_H

#include <QString>

#include <akonadi/collection.h>
#include <akonadi/item.h>

class FakeServerData;

class FakeAkonadiServerCommand
{
public:
  enum Type
  {
    Notification,
    RespondToCollectionFetch,
    RespondToItemFetch
  };

  FakeAkonadiServerCommand( Type type )
    : m_type( type )
  {

  }
  virtual ~FakeAkonadiServerCommand() {}

  Type respondTo() const { return m_type; }
  Akonadi::Collection fetchCollection() const { return m_parentCollection; }

  Type m_type;

  virtual void doCommand() = 0;

  Akonadi::Collection m_parentCollection;
  QHash<Akonadi::Collection::Id, Akonadi::Collection> m_collections;
  QHash<Akonadi::Item::Id, Akonadi::Item> m_items;
  QHash<Akonadi::Item::Id, QList<Akonadi::Entity::Id> > m_childElements;
};

class FakeMonitorCommand : public FakeAkonadiServerCommand
{
public:
  FakeMonitorCommand()
    : FakeAkonadiServerCommand( Notification )
  {

  }
  virtual ~FakeMonitorCommand() {}

  /* reimp */ void doCommand();

};

class FakeJobResponse : public FakeAkonadiServerCommand
{
  struct Token
  {
    enum Type { Branch, Leaf };
    Type type;
    QString content;
  };
public:
  FakeJobResponse( Akonadi::Collection parentCollection, Type respondTo )
    : FakeAkonadiServerCommand( respondTo )
  {
    m_parentCollection = parentCollection;
  }
  virtual ~FakeJobResponse() {}

  void appendCollection( const Akonadi::Collection &collection ) {
    m_collections.insert( collection.id(), collection );
    m_childElements[ collection.parentCollection().id() ].append( collection.id() );
  }
  void appendItem( const Akonadi::Item &item ) {
    m_items.insert( item.id(), item );
  }

  /* reimp */ void doCommand();

  static QList<FakeAkonadiServerCommand*> interpret( FakeServerData *fakeServerData, const QString &input );

private:
  static QList<FakeJobResponse *> parseTreeString( FakeServerData *fakeServerData, const QString& treeString);
  static QList<FakeJobResponse::Token> tokenize( const QString& treeString);
  static void parseEntityString( QList<FakeJobResponse *> &list, QHash<Akonadi::Collection::Id, FakeJobResponse *> &itemResponseMap, Akonadi::Collection::List &recentCollections, FakeServerData *fakeServerData, const QString& entityString, int depth);
};

#endif
