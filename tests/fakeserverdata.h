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

#ifndef FAKE_SERVER_DATA_H
#define FAKE_SERVER_DATA_H

#include <QSharedPointer>

#include <akonadi/job.h>

#include "fakeserver.h"
#include "fakesession.h"
#include "fakemonitor.h"
#include "public_etm.h"

         class QLocalServer;
using namespace Akonadi;

class FakeServerData : public QObject
{
  struct Token
  {
    enum Type { Branch, Leaf };
    Type type;
    QString content;
  };
  Q_OBJECT
public:
  FakeServerData( PublicETM *model, FakeSession *session, FakeMonitor *monitor, QObject *parent = 0 );

  void interpret( const QString &serverData );

signals:
  void emit_itemsFetched( const Akonadi::Item::List &list );

private slots:
  void jobAdded( Akonadi::Job *job );

private:
  void parseTreeString(const QString& treeString);
  QList<FakeServerData::Token> tokenize(const QString& treeString);
  void parseEntityString(const QString& entityString, int depth);

private:
  PublicETM *m_model;
  FakeSession *m_session;
  FakeMonitor *m_monitor;

  QHash<Collection::Id, Collection> m_collections;
  QHash<Item::Id, Item> m_items;
  QHash<Collection::Id, QList<Entity::Id> > m_childElements;

  Collection::List m_recentCollections;
  QList<Collection::List> m_collectionSequence;
  QList<QHash<Item::Id, Item> > m_itemSequence;
  FakeAkonadiServer *m_fakeServer;
  int m_jobsActioned;
  QString m_serverDataString;

  mutable Entity::Id m_nextCollectionId;
  mutable Entity::Id m_nextItemId;


};

#endif
