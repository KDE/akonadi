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

#include "contactgroupviewer.h"

#include "contactgroupexpandjob.h"
#include "standardcontactgroupformatter.h"
#include "textbrowser_p.h"

#include <akonadi/collectionfetchjob.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <kabc/addressee.h>
#include <kabc/contactgroup.h>
#include <kcolorscheme.h>
#include <kicon.h>
#include <klocalizedstring.h>
#include <kstringhandler.h>

#include <QVBoxLayout>

using namespace Akonadi;

class ContactGroupViewer::Private
{
  public:
    Private( ContactGroupViewer *parent )
      : mParent( parent ), mExpandJob( 0 ), mParentCollectionFetchJob( 0 )
    {
      mBrowser = new TextBrowser;

      static QPixmap groupPixmap = KIcon( QLatin1String( "x-mail-distribution-list" ) ).pixmap( QSize( 100, 100 ) );
      mBrowser->document()->addResource( QTextDocument::ImageResource,
                                         QUrl( QLatin1String( "group_photo" ) ),
                                         groupPixmap );

      mStandardContactGroupFormatter = new StandardContactGroupFormatter;
      mContactGroupFormatter = mStandardContactGroupFormatter;
    }

    ~Private()
    {
      delete mStandardContactGroupFormatter;
    }

    void updateView()
    {
      mParent->setWindowTitle( i18n( "Contact Group %1", mCurrentGroupName ) );

      KABC::ContactGroup group;
      group.setName( mCurrentGroupName );
      foreach ( const KABC::Addressee &contact, mCurrentContacts ) {
        group.append( KABC::ContactGroup::Data( contact.realName(), contact.preferredEmail() ) );
      }

      mContactGroupFormatter->setContactGroup( group );

      QList<QVariantMap> additionalFields;

      if ( !mCurrentAddressBookName.isEmpty() ) {
        QVariantMap addressBookName;
        addressBookName.insert( QLatin1String( "title" ), i18n( "Address Book" ) );
        addressBookName.insert( QLatin1String( "value" ), mCurrentAddressBookName );

        additionalFields << addressBookName;
      }

      mContactGroupFormatter->setAdditionalFields( additionalFields );

      mBrowser->setHtml( mContactGroupFormatter->toHtml() );
    }

    void slotMailClicked( const QString&, const QString &email )
    {
      QString name, address;

      // remove the 'mailto:' and split into name and email address
      KABC::Addressee::parseEmailAddress( email.mid( 7 ), name, address );

      emit mParent->emailClicked( name, address );
    }

    void _k_expandResult( KJob *job )
    {
      mExpandJob = 0;

      if ( !job->error() ) {
        ContactGroupExpandJob *expandJob = qobject_cast<ContactGroupExpandJob*>( job );
        mCurrentContacts = expandJob->contacts();
      }

      // stop any running fetch job
      if ( mParentCollectionFetchJob ) {
        mParent->disconnect( mParentCollectionFetchJob, SIGNAL(result(KJob*)), mParent, SLOT(slotParentCollectionFetched(KJob*)) );
        delete mParentCollectionFetchJob;
        mParentCollectionFetchJob = 0;
      }

      mParentCollectionFetchJob = new CollectionFetchJob( mCurrentItem.parentCollection(), CollectionFetchJob::Base, mParent );
      mParent->connect( mParentCollectionFetchJob, SIGNAL(result(KJob*)), SLOT(slotParentCollectionFetched(KJob*)) );
    }

    void slotParentCollectionFetched( KJob *job )
    {
      mParentCollectionFetchJob = 0;
      mCurrentAddressBookName.clear();

      if ( !job->error() ) {
        CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob*>( job );
        if ( !fetchJob->collections().isEmpty() ) {
          const Collection collection = fetchJob->collections().first();
          mCurrentAddressBookName = collection.displayName();
        }
      }

      updateView();
    }

    ContactGroupViewer *mParent;
    TextBrowser *mBrowser;
    QString mCurrentGroupName;
    KABC::Addressee::List mCurrentContacts;
    QString mCurrentAddressBookName;
    Item mCurrentItem;
    ContactGroupExpandJob *mExpandJob;
    CollectionFetchJob *mParentCollectionFetchJob;
    AbstractContactGroupFormatter *mStandardContactGroupFormatter;
    AbstractContactGroupFormatter *mContactGroupFormatter;
};

ContactGroupViewer::ContactGroupViewer( QWidget *parent )
  : QWidget( parent ), d( new Private( this ) )
{
  QVBoxLayout *layout = new QVBoxLayout( this );
  layout->setMargin( 0 );

  d->mBrowser->setNotifyClick( true );

  connect( d->mBrowser, SIGNAL(mailClick(QString,QString)),
           this, SLOT(slotMailClicked(QString,QString)) );

  layout->addWidget( d->mBrowser );

  // always fetch full payload for contact groups
  fetchScope().fetchFullPayload();
  fetchScope().setAncestorRetrieval( ItemFetchScope::Parent );
}

ContactGroupViewer::~ContactGroupViewer()
{
  delete d;
}

Akonadi::Item ContactGroupViewer::contactGroup() const
{
  return ItemMonitor::item();
}

void ContactGroupViewer::setContactGroup( const Akonadi::Item &group )
{
  ItemMonitor::setItem( group );
}

void ContactGroupViewer::setContactGroupFormatter( AbstractContactGroupFormatter *formatter )
{
  if ( formatter == 0 ) {
    d->mContactGroupFormatter = d->mStandardContactGroupFormatter;
  } else {
    d->mContactGroupFormatter = formatter;
  }
}

void ContactGroupViewer::itemChanged( const Item &item )
{
  if ( !item.hasPayload<KABC::ContactGroup>() ) {
    return;
  }

  const KABC::ContactGroup group = item.payload<KABC::ContactGroup>();
  d->mCurrentGroupName = group.name();
  d->mCurrentItem = item;

  if ( d->mExpandJob ) {
    disconnect( d->mExpandJob, SIGNAL(result(KJob*)), this, SLOT(_k_expandResult(KJob*)) );
    d->mExpandJob->kill();
  }

  d->mExpandJob = new ContactGroupExpandJob( group );
  connect( d->mExpandJob, SIGNAL(result(KJob*)), SLOT(_k_expandResult(KJob*)) );
  d->mExpandJob->start();
}

void ContactGroupViewer::itemRemoved()
{
  d->mBrowser->clear();
}

#include "moc_contactgroupviewer.cpp"
