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

using namespace Akonadi;

ItemDumper::ItemDumper( const QByteArray &path, const QByteArray &filename, const QByteArray &mimetype )
{
  QFile f( filename );
  Q_ASSERT( f.open(QIODevice::ReadOnly) );
  QByteArray data = f.readAll();
  f.close();
  ItemAppendJob *job = new ItemAppendJob( path, data, mimetype, this );
  connect( job, SIGNAL(result(KJob*)), SLOT(result(KJob*)) );
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

static KCmdLineOptions options[] =
{
  { "path <argument>", "IMAP destination path", 0 },
  { "mimetype <argument>", "Source mimetype", 0 },
  { "file <argument>", "Source file", 0 },
  KCmdLineLastOption
};

int main( int argc, char** argv )
{
  KCmdLineArgs::init( argc, argv, "test", "Test" ,"test app" ,"1.0" );
  KCmdLineArgs::addCmdLineOptions( options );
  KApplication app;
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  QByteArray path = args->getOption( "path" );
  QByteArray mimetype = args->getOption( "mimetype" );
  QByteArray file = args->getOption( "file" );
  ItemDumper d( path, file, mimetype );
  return app.exec();
}

#include "itemdumper.moc"
