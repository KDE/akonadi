/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>
    Copyright (c) 2007 Robert Zwerus <arzie@dds.nl>

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

#include "itemdumper.h"
#include "collectionpathresolver.h"

#include "item.h"

#include <QDebug>
#include <QFile>
#include <QApplication>

#include <klocale.h>
#include <kapplication.h>
#include <kcmdlineargs.h>

#include "transactionjobs.h"
#include "itemcreatejob.h"

#define GLOBAL_TRANSACTION 1

using namespace Akonadi;

ItemDumper::ItemDumper( const QString &path, const QString &filename, const QString &mimetype, int count ) :
    mJobCount( 0 )
{
  CollectionPathResolver* resolver = new CollectionPathResolver( path, this );
  Q_ASSERT( resolver->exec() );
  const Collection collection = Collection( resolver->collection() );

  QFile f( filename );
  Q_ASSERT( f.open(QIODevice::ReadOnly) );
  QByteArray data = f.readAll();
  f.close();
  Item item;
  item.setMimeType( mimetype );
  item.setPayloadFromData( data );
  mTime.start();
#ifdef GLOBAL_TRANSACTION
  TransactionBeginJob *begin = new TransactionBeginJob( this );
  connect( begin, SIGNAL(result(KJob*)), SLOT(done(KJob*)) );
  ++mJobCount;
#endif
  for ( int i = 0; i < count; ++i ) {
    ++mJobCount;
    ItemCreateJob *job = new ItemCreateJob( item, collection, this );
    connect( job, SIGNAL(result(KJob*)), SLOT(done(KJob*)) );
  }
#ifdef GLOBAL_TRANSACTION
  TransactionCommitJob *commit = new TransactionCommitJob( this );
  connect( commit, SIGNAL(result(KJob*)), SLOT(done(KJob*)) );
  ++mJobCount;
#endif
}

void ItemDumper::done( KJob * job )
{
  --mJobCount;
  if ( job->error() ) {
    qWarning() << "Error while creating item: " << job->errorString();
  }
  if ( mJobCount <= 0 ) {
    qDebug() << "Time:" << mTime.elapsed() << "ms";
    qApp->quit();
  }
}

int main( int argc, char** argv )
{
  KCmdLineArgs::init( argc, argv, "test", 0, ki18n("Test Application") ,"1.0" ,ki18n("test app") );

  KCmdLineOptions options;
  options.add("path <argument>", ki18n("IMAP destination path"));
  options.add("mimetype <argument>", ki18n("Source mimetype"), "application/octet-stream");
  options.add("file <argument>", ki18n("Source file"));
  options.add("count <argument>", ki18n("Number of times this file is added"), "1");
  KCmdLineArgs::addCmdLineOptions( options );
  KApplication app;
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  QString path = args->getOption( "path" );
  QString mimetype = args->getOption( "mimetype" );
  QString file = args->getOption( "file" );
  int count = qMax( 1, args->getOption( "count").toInt() );
  ItemDumper d( path, file, mimetype, count );
  return app.exec();
}

