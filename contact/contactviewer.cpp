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

#include "contactviewer.h"

#include "contactmetadata_p.h"
#include "contactmetadataattribute_p.h"
#include "customfieldmanager_p.h"
#include "standardcontactformatter.h"
#include "textbrowser_p.h"

#include <akonadi/item.h>
#include <akonadi/itemfetchscope.h>
#include <kabc/addressee.h>
#include <kcolorscheme.h>
#include <kglobal.h>
#include <kicon.h>
#include <klocale.h>
#include <kstringhandler.h>

#include <QtGui/QVBoxLayout>

using namespace Akonadi;

class ContactViewer::Private
{
  public:
    Private( ContactViewer *parent )
      : mParent( parent )
    {
      mContactFormatter = new StandardContactFormatter;
    }

    ~Private()
    {
      delete mContactFormatter;
    }

    void updateView( const QVariantList &localCustomFieldDescriptions = QVariantList() )
    {
      static QPixmap defaultPixmap = KIcon( QLatin1String( "user-identity" ) ).pixmap( QSize( 100, 140 ) );

      mParent->setWindowTitle( i18n( "Contact %1", mCurrentContact.assembledName() ) );

      if ( mCurrentContact.photo().isIntern() ) {
        mBrowser->document()->addResource( QTextDocument::ImageResource,
                                           QUrl( QLatin1String( "contact_photo" ) ),
                                           mCurrentContact.photo().data() );
      } else {
        mBrowser->document()->addResource( QTextDocument::ImageResource,
                                           QUrl( QLatin1String( "contact_photo" ) ),
                                           defaultPixmap );
      }

      // merge local and global custom field descriptions
      QVariantList customFieldDescriptions( localCustomFieldDescriptions );

      const CustomField::List globalCustomFields = CustomFieldManager::globalCustomFieldDescriptions();
      foreach ( const CustomField &field, globalCustomFields ) {
        QVariantMap description;
        description.insert( QLatin1String( "key" ), field.key() );
        description.insert( QLatin1String( "title" ), field.title() );

        customFieldDescriptions << description;
      }

      mContactFormatter->setContact( mCurrentContact );
      mContactFormatter->setCustomFieldDescriptions( customFieldDescriptions );

      mBrowser->setHtml( mContactFormatter->toHtml() );
    }

    void slotMailClicked( const QString&, const QString &email )
    {
      QString name, address;

      // remove the 'mailto:' and split into name and email address
      KABC::Addressee::parseEmailAddress( email.mid( 7 ), name, address );

      emit mParent->emailClicked( name, address );
    }

    void slotUrlClicked( const QString &urlString )
    {
      KUrl url( urlString );

      if ( url.scheme() == QLatin1String( "http" ) ||
           url.scheme() == QLatin1String( "https" ) ) {
        emit mParent->urlClicked( url );
      } else if ( url.scheme() == QLatin1String( "phone" ) ) {
        const int pos = url.queryItemValue( QLatin1String( "index" ) ).toInt();

        const KABC::PhoneNumber::List numbers = mCurrentContact.phoneNumbers();
        if ( pos < numbers.count() ) {
          emit mParent->phoneNumberClicked( numbers.at( pos ) );
        }
      } else if ( url.scheme() == QLatin1String( "address" ) ) {
        const int pos = url.queryItemValue( QLatin1String( "index" ) ).toInt();

        const KABC::Address::List addresses = mCurrentContact.addresses();
        if ( pos < addresses.count() ) {
          emit mParent->addressClicked( addresses.at( pos ) );
        }
      }
    }

    ContactViewer *mParent;
    TextBrowser *mBrowser;
    KABC::Addressee mCurrentContact;
    AbstractContactFormatter *mContactFormatter;
};

ContactViewer::ContactViewer( QWidget *parent )
  : QWidget( parent ), d( new Private( this ) )
{
  QVBoxLayout *layout = new QVBoxLayout( this );
  layout->setMargin( 0 );

  d->mBrowser = new TextBrowser;
  d->mBrowser->setNotifyClick( true );

  connect( d->mBrowser, SIGNAL( mailClick( const QString&, const QString& ) ),
           this, SLOT( slotMailClicked( const QString&, const QString& ) ) );
  connect( d->mBrowser, SIGNAL( urlClick( const QString& ) ),
           this, SLOT( slotUrlClicked( const QString& ) ) );

  layout->addWidget( d->mBrowser );

  // always fetch full payload for contacts
  fetchScope().fetchFullPayload();
  fetchScope().fetchAttribute<ContactMetaDataAttribute>();
}

ContactViewer::~ContactViewer()
{
  delete d;
}

Akonadi::Item ContactViewer::contact() const
{
  return ItemMonitor::item();
}

KABC::Addressee ContactViewer::rawContact() const
{
  return d->mCurrentContact;
}

void ContactViewer::setContact( const Akonadi::Item &contact )
{
  ItemMonitor::setItem( contact );
}

void ContactViewer::setRawContact( const KABC::Addressee &contact )
{
  d->mCurrentContact = contact;

  d->updateView();
}

void ContactViewer::itemChanged( const Item &contactItem )
{
  if ( !contactItem.hasPayload<KABC::Addressee>() )
    return;

  d->mCurrentContact = contactItem.payload<KABC::Addressee>();

  // load the local meta data of the item
  ContactMetaData metaData;
  metaData.load( contactItem );

  d->updateView( metaData.customFieldDescriptions() );
}

void ContactViewer::itemRemoved()
{
  d->mBrowser->clear();
}

#include "contactviewer.moc"
