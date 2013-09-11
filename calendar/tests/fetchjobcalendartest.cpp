/*
    Copyright (c) 2011 Sérgio Martins <iamsergio@gmail.com>

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

#include "../fetchjobcalendar.h"
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/qtest_akonadi.h>

#include <QTestEventLoop>

using namespace Akonadi;
using namespace KCalCore;

class FetchJobCalendarTest : public QObject
{
  Q_OBJECT
    Collection mCollection;

    void createIncidence( const QString &uid )
    {
      Item item;
      item.setMimeType( Event::eventMimeType() );
      Incidence::Ptr incidence = Incidence::Ptr( new Event() );
      incidence->setUid( uid );
      incidence->setSummary( QLatin1String( "summary" ) );
      item.setPayload<KCalCore::Incidence::Ptr>( incidence );
      ItemCreateJob *job = new ItemCreateJob( item, mCollection, this );
      AKVERIFYEXEC( job );
    }

    void fetchCollection()
    {
      CollectionFetchJob *job = new CollectionFetchJob( Collection::root(),
                                                        CollectionFetchJob::Recursive,
                                                        this );
      // Get list of collections
      job->fetchScope().setContentMimeTypes( QStringList() << QLatin1String( "application/x-vnd.akonadi.calendar.event" ) );
      AKVERIFYEXEC( job );

      // Find our collection
      Collection::List collections = job->collections();
      QVERIFY( !collections.isEmpty() );
      mCollection = collections.first();

      QVERIFY( mCollection.isValid() );
    }
private Q_SLOTS:
    void initTestCase()
    {
      fetchCollection();
      qRegisterMetaType<Akonadi::Item>("Akonadi::Item");
    }

    void testFetching()
    {
      createIncidence( tr( "a" ) );
      createIncidence( tr( "b" ) );
      createIncidence( tr( "c" ) );
      createIncidence( tr( "d" ) );
      createIncidence( tr( "e" ) );
      createIncidence( tr( "f" ) );

      FetchJobCalendar *calendar = new FetchJobCalendar();
      connect( calendar, SIGNAL(loadFinished(bool,QString)),
               SLOT(handleLoadFinished(bool,QString)) );
      QTestEventLoop::instance().enterLoop( 10 );
      QVERIFY( !QTestEventLoop::instance().timeout() );

      Incidence::List incidences = calendar->incidences();
      QVERIFY( incidences.count() == 6 );
      QVERIFY( calendar->item( tr( "a" ) ).isValid() );
      QVERIFY( calendar->item( tr( "b" ) ).isValid() );
      QVERIFY( calendar->item( tr( "c" ) ).isValid() );
      QVERIFY( calendar->item( tr( "d" ) ).isValid() );
      QVERIFY( calendar->item( tr( "e" ) ).isValid() );
      QVERIFY( calendar->item( tr( "f" ) ).isValid() );

      delete calendar;
    }

public Q_SLOTS:
  void handleLoadFinished( bool success, const QString &errorMessage )
  {
    if ( !success )
      qDebug() << errorMessage;
    QVERIFY( success );
    QTestEventLoop::instance().exitLoop();
  }
};

QTEST_AKONADIMAIN( FetchJobCalendarTest, GUI )

#include "fetchjobcalendartest.moc"
