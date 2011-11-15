/*
    Copyright (c) 2010-2011 Sérgio Martins <iamsergio@gmail.com>

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

#include "../calendarbase.h"

#include <akonadi/qtest_akonadi.h>
#include <Akonadi/Collection>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/ItemCreateJob>

#include <QtCore/QObject>

using namespace Akonadi;
using namespace KCalCore;

class CalendarBaseTest : public QObject
{
  Q_OBJECT
  Collection mCollection;
  CalendarBase *mCalendar;

  bool mExpectedResult;

  private slots:

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

    void createInitialIncidences()
    {
      Item::List items;

      for( int i=0; i<5; ++i ) {
        Item item;
        item.setMimeType( Event::eventMimeType() );
        Incidence::Ptr incidence = Incidence::Ptr( new Event() );
        incidence->setUid( QLatin1String( "event" ) + QString::number( i ) );
        incidence->setSummary( QLatin1String( "summary" ) + QString::number( i ) );
        item.setPayload<KCalCore::Incidence::Ptr>( incidence );
        items.append( item );
      }

      for( int i=0; i<5; ++i ) {
        Item item;
        item.setMimeType( Todo::todoMimeType() );
        Incidence::Ptr incidence = Incidence::Ptr( new Todo() );
        incidence->setUid( QLatin1String( "todo" ) + QString::number( i ) );
        incidence->setSummary( QLatin1String( "summary" ) + QString::number( i ) );
        item.setPayload<KCalCore::Incidence::Ptr>( incidence );
        items.append( item );
      }

      for( int i=0; i<5; ++i ) {
        Item item;
        item.setMimeType( Journal::journalMimeType() );
        Incidence::Ptr incidence = Incidence::Ptr( new Journal() );
        incidence->setUid( QLatin1String( "journal" ) + QString::number( i ) );
        incidence->setSummary( QLatin1String( "summary" ) + QString::number( i ) );
        item.setPayload<KCalCore::Incidence::Ptr>( incidence );
        items.append( item );
      }
      
      foreach( const Akonadi::Item &item, items ) {
        ItemCreateJob *job = new ItemCreateJob( item, mCollection, this );
        AKVERIFYEXEC( job );
      }
    }

    void initTestCase()
    {
      fetchCollection();
      createInitialIncidences();
      mCalendar = new CalendarBase();
      connect( mCalendar, SIGNAL(createFinished(bool,QString)),
               SLOT(handleCreateFinished(bool,QString)) );

      connect( mCalendar, SIGNAL(deleteFinished(bool,QString)),
               SLOT(handleDeleteFinished(bool,QString)) );
    }

    void cleanupTestCase()
    {
      delete mCalendar;
    }

public Q_SLOTS:
    void handleCreateFinished( bool success, const QString &errorString )
    {
      Q_UNUSED( success );
      Q_UNUSED( errorString );
    }

    void handleDeleteFinished( bool success, const QString &errorString )
    {
      Q_UNUSED( success );
      Q_UNUSED( errorString );
    }
};

QTEST_AKONADIMAIN( CalendarBaseTest, GUI )

#include "calendarbasetest.moc"
