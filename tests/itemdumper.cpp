/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtGui/QApplication>

#include <kapplication.h>
#include <kcmdlineargs.h>

#include <libakonadi/collectionpathresolver.h>
#include <libakonadi/itemserializer.h>

using namespace Akonadi;

ItemDumper::ItemDumper( const QString &path, const QString &filename, const QString &mimetype )
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
  ItemSerializer::deserialize( item, Item::PartBody, data );
  ItemAppendJob *job = new ItemAppendJob( item, collection, this );
  connect( job, SIGNAL(result(KJob*)), SLOT(done(KJob*)) );
  job->start();
}

void ItemDumper::done( KJob * job )
{
  if ( job->error() ) {
    qWarning() << "Error while creating item: " << job->errorString();
  } else {
    qDebug() << "Done!";
  }
  qApp->quit();
}

int main( int argc, char** argv )
{
  KCmdLineArgs::init( argc, argv, "test", 0, ki18n("Test") ,"1.0" ,ki18n("test app") );

  KCmdLineOptions options;
  options.add("path <argument>", ki18n("IMAP destination path"));
  options.add("mimetype <argument>", ki18n("Source mimetype"));
  options.add("file <argument>", ki18n("Source file"));
  KCmdLineArgs::addCmdLineOptions( options );
  KApplication app;
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  QString path = args->getOption( "path" );
  QString mimetype = args->getOption( "mimetype" );
  QString file = args->getOption( "file" );
  ItemDumper d( path, file, mimetype );
  return app.exec();
}

#include "itemdumper.moc"
