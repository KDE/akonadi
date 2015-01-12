/*
    This file is part of Akonadi Contact.

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

#include <akonadi/contact/contactgroupsearchjob.h>
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

    Private( const QString &name, ContactGroupExpandJob *parent )
      : mParent( parent ), mName( name ), mFetchCount( 0 )
    {
    }

    void resolveGroup()
    {
      for ( unsigned int i = 0; i < mGroup.dataCount(); ++i ) {
        const KABC::ContactGroup::Data data = mGroup.data( i );

        KABC::Addressee contact;
        contact.setNameFromString( data.name() );
        contact.insertEmail( data.email(), true );

        mContacts.append( contact );
      }

      for ( unsigned int i = 0; i < mGroup.contactReferenceCount(); ++i ) {
        const KABC::ContactGroup::ContactReference reference = mGroup.contactReference( i );

        Item item;
        if ( !reference.gid().isEmpty() ) {
          item.setGid( reference.gid() );
        } else {
          item.setId( reference.uid().toLongLong() );
        }
        ItemFetchJob *job = new ItemFetchJob( item, mParent );
        job->fetchScope().fetchFullPayload();
        job->setProperty( "preferredEmail", reference.preferredEmail() );

        mParent->connect( job, SIGNAL(result(KJob*)), mParent, SLOT(fetchResult(KJob*)) );

        mFetchCount++;
      }

      if ( mFetchCount == 0 ) { // nothing to fetch, so we can return immediately
        mParent->emitResult();
      }
    }

    void searchResult( KJob *job )
    {
      if ( job->error() ) {
        mParent->setError( job->error() );
        mParent->setErrorText( job->errorText() );
        mParent->emitResult();
        return;
      }

      ContactGroupSearchJob *searchJob = qobject_cast<ContactGroupSearchJob*>( job );

      if ( searchJob->contactGroups().isEmpty() ) {
        mParent->emitResult();
        return;
      }

      mGroup = searchJob->contactGroups().first();
      resolveGroup();
    }

    void fetchResult( KJob *job )
    {
      const ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob*>( job );

      const Item::List items = fetchJob->items();
      if ( !items.isEmpty() ) {
        const QString email = fetchJob->property( "preferredEmail" ).toString();

        const Item item = items.first();
        if ( item.hasPayload<KABC::Addressee>() ) {
          KABC::Addressee contact = item.payload<KABC::Addressee>();
          if ( !email.isEmpty() ) {
            contact.insertEmail( email, true );
          }

          mContacts.append( contact );
        } else
          kWarning() << "Contact for Akonadi item" << item.id() << "does not exist anymore!";
      }

      mFetchCount--;

      if ( mFetchCount == 0 ) {
        mParent->emitResult();
      }
    }

    ContactGroupExpandJob *mParent;
    KABC::ContactGroup mGroup;
    QString mName;
    KABC::Addressee::List mContacts;

    int mFetchCount;
};

ContactGroupExpandJob::ContactGroupExpandJob( const KABC::ContactGroup &group, QObject * parent )
  : KJob( parent ), d( new Private( group, this ) )
{
}

ContactGroupExpandJob::ContactGroupExpandJob( const QString &name, QObject * parent )
  : KJob( parent ), d( new Private( name, this ) )
{
}

ContactGroupExpandJob::~ContactGroupExpandJob()
{
  delete d;
}

void ContactGroupExpandJob::start()
{
  if ( !d->mName.isEmpty() && !d->mName.contains( QLatin1Char( '@' ) ) ) {
    // we have to search the contact group first
    ContactGroupSearchJob *searchJob = new ContactGroupSearchJob( this );
    searchJob->setQuery( ContactGroupSearchJob::Name, d->mName );
    searchJob->setLimit( 1 );
    connect( searchJob, SIGNAL(result(KJob*)), this, SLOT(searchResult(KJob*)) );
  } else {
    QMetaObject::invokeMethod(this, "resolveGroup", Qt::QueuedConnection);
  }
}

KABC::Addressee::List ContactGroupExpandJob::contacts() const
{
  return d->mContacts;
}

#include "moc_contactgroupexpandjob.cpp"
