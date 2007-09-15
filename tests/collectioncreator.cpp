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

#include "collectioncreator.h"
#include "collectioncreatejob.h"
#include "collectionpathresolver.h"

#include <QtCore/QDebug>
#include <QtGui/QApplication>

#include <kapplication.h>
#include <kcmdlineargs.h>

#define COLLECTION_COUNT 1000

using namespace Akonadi;

CollectionCreator::CollectionCreator( )
{
  jobCount = 0;
  CollectionPathResolver *resolver = new CollectionPathResolver( "res3", this );
  if ( !resolver->exec() )
    qFatal( "Cannot resolve path." );
  int root = resolver->collection();
  startTime.start();
  for ( int i = 0; i < COLLECTION_COUNT; ++i ) {
    CollectionCreateJob *job = new CollectionCreateJob( Collection( root ),
        QLatin1String("col") + QString::number( i ), this );
    connect( job, SIGNAL(result(KJob*)), SLOT(done(KJob*)) );
    ++jobCount;
  }
}

void CollectionCreator::done( KJob * job )
{
  if ( job->error() )
    qWarning() << "collection creation failed: " << job->errorString();
  --jobCount;
  if ( jobCount <= 0 ) {
    qDebug() << "creation took: " << startTime.elapsed() << "ms.";
    qApp->quit();
  }
}


int main( int argc, char** argv )
{
  KCmdLineArgs::init( argc, argv, "test", 0, ki18n("Test") ,"1.0" ,ki18n("test app") );
  KApplication app;
  CollectionCreator *cc = new CollectionCreator();
  return app.exec();
  delete cc;
}

#include "collectioncreator.moc"
