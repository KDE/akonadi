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
#include <akonadi/entitytreemodel.h>
#include <akonadi/item.h>

class FakeServerData;

class FakeAkonadiServerCommand : public QObject
{
  Q_OBJECT
public:
  enum Type
  {
    Notification,
    RespondToCollectionFetch,
    RespondToItemFetch
  };

  FakeAkonadiServerCommand( Type type, Akonadi::EntityTreeModel *model );

  virtual ~FakeAkonadiServerCommand() {}

  Type respondTo() const { return m_type; }
  Akonadi::Collection fetchCollection() const { return m_parentCollection; }

  Type m_type;

  virtual void doCommand() = 0;

signals:
  void emit_itemsFetched( const Akonadi::Item::List &list );
  void emit_collectionsFetched( const Akonadi::Collection::List &list );
  void emit_collectionMoved( const Akonadi::Collection &collection, const Akonadi::Collection &source, const Akonadi::Collection &target );
  void emit_itemMoved( const Akonadi::Item &item, const Akonadi::Collection &source, const Akonadi::Collection &target );

protected:
  Akonadi::Collection getCollectionByDisplayName( const QString &displayName ) const;
  Akonadi::Item getItemByDisplayName( const QString &displayName ) const;

protected:
  Akonadi::EntityTreeModel *m_model;
  Akonadi::Collection m_parentCollection;
  QHash<Akonadi::Collection::Id, Akonadi::Collection> m_collections;
  QHash<Akonadi::Item::Id, Akonadi::Item> m_items;
  QHash<Akonadi::Item::Id, QList<Akonadi::Entity::Id> > m_childElements;
};

class FakeMonitorCommand : public FakeAkonadiServerCommand
{
public:
  explicit FakeMonitorCommand( Akonadi::EntityTreeModel *model )
    : FakeAkonadiServerCommand( Notification, model )
  {

  }
  virtual ~FakeMonitorCommand() {}
};

class FakeCollectionMovedCommand : public FakeMonitorCommand
{
public:
  FakeCollectionMovedCommand( const QString &collection, const QString &source, const QString &target, Akonadi::EntityTreeModel *model )
    : FakeMonitorCommand( model ), m_collectionName( collection ), m_sourceName( source ), m_targetName( target )
  {

  }

  virtual ~FakeCollectionMovedCommand() {}

  /* reimp */ void doCommand();

private:
  QString m_collectionName;
  QString m_sourceName;
  QString m_targetName;
  FakeServerData *m_serverData;
};

class FakeItemMovedCommand : public FakeMonitorCommand
{
public:
  FakeItemMovedCommand( const QString &item, const QString &source, const QString &target, Akonadi::EntityTreeModel *model )
    : FakeMonitorCommand( model ), m_itemName( item ), m_sourceName( source ), m_targetName( target )
  {

  }

  virtual ~FakeItemMovedCommand() {}

  /* reimp */ void doCommand();

private:
  QString m_itemName;
  QString m_sourceName;
  QString m_targetName;
  FakeServerData *m_serverData;
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
  FakeJobResponse( Akonadi::Collection parentCollection, Type respondTo, Akonadi::EntityTreeModel *model )
    : FakeAkonadiServerCommand( respondTo, model )
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
