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

#include "collectioncreator.h"
#include "collectioncreatejob.h"

#include <QtCore/QDebug>
#include <QtGui/QApplication>

#include <kapplication.h>
#include <kcmdlineargs.h>

#define COLLECTION_COUNT 100
#undef SEQUENTIAL_EXECUTION

using namespace Akonadi;

CollectionCreator::CollectionCreator( )
{
#ifdef SEQUENTIAL_EXECUTION
  queue = new JobQueue( this );
#else
  jobCount = 0;
#endif
  startTime = QTime::currentTime();
  for ( int i = 0; i < COLLECTION_COUNT; ++i ) {
#ifdef SEQUENTIAL_EXECUTION
    CollectionCreateJob *job = new CollectionCreateJob( "res3/col" + QByteArray::number( i ), queue );
    connect( job, SIGNAL(done(Akonadi::Job*)), SLOT(done(Akonadi::Job*)) );
    queue->addJob( job );
#else
    CollectionCreateJob *job = new CollectionCreateJob( "res3/col" + QByteArray::number( i ), this );
    connect( job, SIGNAL(done(Akonadi::Job*)), SLOT(done(Akonadi::Job*)) );
    job->start();
    ++jobCount;
#endif
  }
}

void CollectionCreator::done( Akonadi::Job * job )
{
#ifndef SEQUENTIAL_EXECUTION
  --jobCount;
#endif
  if ( job->error() ) {
    qWarning() << "Creation failed: " << job->errorText();
  }
  job->deleteLater();
#ifdef SEQUENTIAL_EXECUTION
  if ( queue->isEmpty() ) {
#else
  if ( jobCount <= 0 ) {
#endif
    qDebug() << "creation took: " << startTime.secsTo( QTime::currentTime() ) << " seconds.";
    qApp->quit();
  }
}


int main( int argc, char** argv )
{
  KCmdLineArgs::init( argc, argv, "test", "Test" ,"test app" ,"1.0" );
  KApplication app;
  CollectionCreator *cc = new CollectionCreator();
  return app.exec();
  delete cc;
}

#include "collectioncreator.moc"
