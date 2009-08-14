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

#include "contactgroupexpandjob.h"

#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemsearchjob.h>

using namespace Akonadi;

class ContactGroupExpandJob::Private
{
  public:
    Private( const KABC::ContactGroup &group, ContactGroupExpandJob *parent )
      : mParent( parent ), mGroup( group ), mFetchCount( 0 )
    {
    }

    void fetchResult( KJob *job )
    {
      const ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob*>( job );

      const Item::List items = fetchJob->items();
      if ( !items.isEmpty() ) {
        const QString email = fetchJob->property( "preferredEmail" ).toString();

        KABC::Addressee contact = items.first().payload<KABC::Addressee>();
        if ( !email.isEmpty() )
          contact.insertEmail( email, true );

        mContacts.append( contact );
      }

      mFetchCount--;

      if ( mFetchCount == 0 )
        mParent->emitResult();
    }

    ContactGroupExpandJob *mParent;
    KABC::ContactGroup mGroup;
    KABC::Addressee::List mContacts;

    int mFetchCount;
};

ContactGroupExpandJob::ContactGroupExpandJob( const KABC::ContactGroup &group, QObject * parent )
  : KJob( parent ), d( new Private( group, this ) )
{
}

ContactGroupExpandJob::~ContactGroupExpandJob()
{
  delete d;
}

void ContactGroupExpandJob::start()
{
  for ( unsigned int i = 0; i < d->mGroup.dataCount(); ++i ) {
    const KABC::ContactGroup::Data data = d->mGroup.data( i );

    KABC::Addressee contact;
    contact.setNameFromString( data.name() );
    contact.insertEmail( data.email(), true );

    d->mContacts.append( contact );
  }

  for ( unsigned int i = 0; i < d->mGroup.contactReferenceCount(); ++i ) {
    const KABC::ContactGroup::ContactReference reference = d->mGroup.contactReference( i );

    ItemFetchJob *job = new ItemFetchJob( Item( reference.uid().toLongLong() ) );
    job->fetchScope().fetchFullPayload();
    job->setProperty( "preferredEmail", reference.preferredEmail() );

    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( fetchResult( KJob* ) ) );

    d->mFetchCount++;
  }

  if ( d->mFetchCount == 0 ) // nothing to fetch, so we can return immediately
    emitResult();
}

KABC::Addressee::List ContactGroupExpandJob::contacts() const
{
  return d->mContacts;
}

QStringList ContactGroupExpandJob::emailAddresses() const
{
  QStringList emails;

  foreach ( const KABC::Addressee &contact, d->mContacts )
    emails.append( contact.fullEmail() );

  return emails;
}

#include "contactgroupexpandjob.moc"
