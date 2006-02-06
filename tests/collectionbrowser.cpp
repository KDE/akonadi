/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include "collection.h"
#include "collectionbrowser.h"
#include "collectionlistjob.h"
#define private public
#include "collectionmodel.h"

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kdebug.h>

#include <QHash>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QTreeView>

#include <stdlib.h>

using namespace PIM;

extern QHash<DataReference, Collection*> global_collection_map;
static int id = 0;

static void createCollections( const DataReference &parent, int rec )
{
  if ( rec <= 0 )
    return;
  int count = rand() % 10;
  for ( int i = 0; i < count; ++i ) {
    DataReference ref( QString::number( id++ ), QString() );
    Collection *col = new Collection( ref );
    col->setParent( parent );
    col->setName( ref.persistanceID() );
    QStringList content;
    switch ( rand() % 6 ) {
      case 1:
        content << "text/x-vcard"; break;
      case 2:
        content << "akonadi/event"; break;
      case 3:
        content << "akonadi/task"; break;
      default:
        content << "message/rfc822";
    }
    col->setContentTypes( content );
    global_collection_map.insert( ref, col );
    if ( parent.isNull() )
      col->setType( Collection::Resource );
    createCollections( ref, rec - ( rand() % 3 ) - 1 );
  }
}


CollectionBrowser::CollectionBrowser() : CollectionView()
{
  CollectionListJob *job = new CollectionListJob();
  job->exec();
  QHash<DataReference, Collection*> collections = job->collections();
  delete job;
  // use some dummy collections for now
  createCollections( DataReference(), 8 );
  foreach ( Collection *col, global_collection_map ) {
    Collection *colCopy = new Collection( col->reference() );
    colCopy->copy( col );
    collections.insert( col->reference(), colCopy );
  }
  model = new CollectionModel( collections, this );
  setModel( model );

  // emulate the monitor job
  QTimer *t = new QTimer( this );
  connect( t, SIGNAL( timeout() ), SLOT( slotEmitChanged() ) );
  t->start( 1000 );
//   t->setSingleShot( true );
}

void CollectionBrowser::slotEmitChanged()
{
  // update
  int id = rand() % 1000;
  DataReference ref( QString::number( id ), QString() );
  Collection *col = global_collection_map.value( ref );
  if ( col ) {
    if ( rand() % 10 ) {
      col->setName( col->name() + " (changed)" );
      kDebug() << k_funcinfo << "update collection " << ref.persistanceID() << " parent: " << col->parent().persistanceID() << endl;
    } else {
      col->setParent( DataReference( QString::number( rand() % 1000 ), QString() ) );
      col->setName( col->name() + " (reparented)" );
      kDebug() << k_funcinfo << "reparenting collection " << ref.persistanceID() << " to new parent " << col->parent().persistanceID() << endl;
    }
    model->collectionChanged( ref );
  }
  // delete
  id = rand() % 1000;
  id += 500;
  ref = DataReference( QString::number( id ), QString() );
  global_collection_map.remove( ref );
  kDebug() << k_funcinfo << "remove collection " << ref.persistanceID() << endl;
  model->collectionRemoved( ref );
  // add
  id = rand() % 1000;
  id += 10000;
  ref = DataReference( QString::number( id ), QString() );
  col = new Collection( ref );
  col->setParent( DataReference( QString::number( rand() % 1000 ), QString() ) );
  col->setName( ref.persistanceID() + " (new)" );
  global_collection_map.insert( ref, col );
  kDebug() << k_funcinfo << "add collection " << ref.persistanceID() << " parent: " << col->parent().persistanceID() << endl;
  model->collectionChanged( ref );
}

int main( int argc, char** argv )
{
  KCmdLineArgs::init( argc, argv, "test", "Test" ,"test app" ,"1.0" );
  KApplication app;
  CollectionBrowser w;
  w.show();
  return app.exec();
}

#include "collectionbrowser.moc"
