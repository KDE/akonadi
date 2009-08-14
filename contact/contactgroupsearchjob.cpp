/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "contactgroupsearchjob.h"

#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemsearchjob.h>

using namespace Akonadi;

class ContactGroupSearchJob::Private
{
  public:
    Private( ContactGroupSearchJob *parent )
      : mParent( parent ), mCriterion( ContactGroupSearchJob::Name )
    {
    }

    void searchResult( KJob *job )
    {
      ItemSearchJob *searchJob = qobject_cast<ItemSearchJob*>( job );
      mItems = searchJob->items();
      mSearchResults = mItems.count();

      if ( mSearchResults == 0 ) {
        mParent->emitResult();
        return;
      }

      foreach ( const Item &item, mItems ) {
        ItemFetchJob *job = new ItemFetchJob( item );
        job->fetchScope().fetchFullPayload();

        mParent->connect( job, SIGNAL( result( KJob* ) ), mParent, SLOT( fetchResult( KJob* ) ) );
      }
    }

    void fetchResult( KJob *job )
    {
      ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob*>( job );

      const Item::List items = fetchJob->items();
      if ( items.count() == 1 ) {
        mContactGroups.append( items.first().payload<KABC::ContactGroup>() );
      }

      mSearchResults--;

      if ( mSearchResults == 0 )
        mParent->emitResult();
    }

    ContactGroupSearchJob *mParent;
    ContactGroupSearchJob::Criterion mCriterion;
    QString mValue;
    KABC::ContactGroup::List mContactGroups;
    Item::List mItems;
    int mSearchResults;
};

ContactGroupSearchJob::ContactGroupSearchJob( QObject * parent )
  : KJob( parent ), d( new Private( this ) )
{
}

ContactGroupSearchJob::~ContactGroupSearchJob()
{
  delete d;
}

void ContactGroupSearchJob::setQuery( Criterion criterion, const QString &value )
{
  d->mCriterion = criterion;
  d->mValue = value;
}

void ContactGroupSearchJob::start()
{
  QString query;

  if ( d->mCriterion == Name ) {
    query = QString::fromLatin1( ""
                                 "prefix nco:<http://www.semanticdesktop.org/ontologies/2007/03/22/nco#>"
                                 "SELECT ?group WHERE {"
                                 "  ?group nco:contactGroupName \"%1\"^^<http://www.w3.org/2001/XMLSchema#string>."
                                 "}" );
  }

  query = query.arg( d->mValue );

  ItemSearchJob *job = new ItemSearchJob( query, this );
  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( searchResult( KJob* ) ) );
  job->start();
}

KABC::ContactGroup::List ContactGroupSearchJob::contactGroups() const
{
  return d->mContactGroups;
}

Item::List ContactGroupSearchJob::items() const
{
  return d->mItems;
}

#include "contactgroupsearchjob.moc"
