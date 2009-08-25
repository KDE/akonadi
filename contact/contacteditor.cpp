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

#include "contacteditor.h"

#include "abstractcontacteditorwidget.h"
#include "contactmetadata_p.h"
#include "contactmetadataattribute_p.h"
#include "addressbookselectiondialog.h"
#include "autoqpointer.h"

#include <akonadi/itemcreatejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>
#include <kabc/addressee.h>
#include <klocale.h>

#include <QtGui/QVBoxLayout>
#include <QtGui/QMessageBox>

using namespace Akonadi;

class ContactEditor::Private
{
  public:
    Private( ContactEditor *parent )
      : mParent( parent ), mMonitor( 0 ), mReadOnly( false )
    {
    }

    ~Private()
    {
      delete mMonitor;
    }

    void fetchDone( KJob* );
    void storeDone( KJob* );
    void itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& );

    void loadContact( const KABC::Addressee &addr, const ContactMetaData &metaData );
    void storeContact( KABC::Addressee &addr, ContactMetaData &metaData );
    void setupMonitor();

    ContactEditor *mParent;
    ContactEditor::Mode mMode;
    Akonadi::Item mItem;
    Akonadi::ContactMetaData mContactMetaData;
    Akonadi::Monitor *mMonitor;
    Akonadi::Collection mDefaultCollection;
    AbstractContactEditorWidget *mEditorWidget;
    bool mReadOnly;
};

void ContactEditor::Private::fetchDone( KJob *job )
{
  if ( job->error() != KJob::NoError )
    return;

  Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob*>( job );
  if ( !fetchJob )
    return;

  if ( fetchJob->items().isEmpty() )
    return;

  mItem = fetchJob->items().first();

  if ( mMode == ContactEditor::EditMode ) {
    mReadOnly = false;

    const Akonadi::Collection parentCollection = mItem.parentCollection();
    if ( parentCollection.isValid() )
      mReadOnly = !(parentCollection.rights() & Collection::CanChangeItem);

      mEditorWidget->setReadOnly( mReadOnly );
  }

  const KABC::Addressee addr = mItem.payload<KABC::Addressee>();
  mContactMetaData.load( mItem );
  loadContact( addr, mContactMetaData );
}

void ContactEditor::Private::storeDone( KJob *job )
{
  if ( job->error() != KJob::NoError ) {
    emit mParent->error( job->errorString() );
    return;
  }

  if ( mMode == EditMode )
    emit mParent->contactStored( mItem );
  else if ( mMode == CreateMode )
    emit mParent->contactStored( static_cast<Akonadi::ItemCreateJob*>( job )->item() );
}

void ContactEditor::Private::itemChanged( const Akonadi::Item&, const QSet<QByteArray>& )
{
  QMessageBox dlg( mParent );

  dlg.setInformativeText( QLatin1String( "The contact has been changed by anyone else\nWhat shall be done?" ) );
  dlg.addButton( i18n( "Take over changes" ), QMessageBox::AcceptRole );
  dlg.addButton( i18n( "Ignore and Overwrite changes" ), QMessageBox::RejectRole );

  if ( dlg.exec() == QMessageBox::AcceptRole ) {
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( mItem );
    job->fetchScope().fetchFullPayload();
    job->fetchScope().fetchAttribute<ContactMetaDataAttribute>();
    job->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );

    mParent->connect( job, SIGNAL( result( KJob* ) ), mParent, SLOT( fetchDone( KJob* ) ) );
  }
}

void ContactEditor::Private::loadContact( const KABC::Addressee &addr, const ContactMetaData &metaData )
{
  mEditorWidget->loadContact( addr, metaData );
}

void ContactEditor::Private::storeContact( KABC::Addressee &addr, ContactMetaData &metaData )
{
  mEditorWidget->storeContact( addr, metaData );
}

void ContactEditor::Private::setupMonitor()
{
  delete mMonitor;
  mMonitor = new Akonadi::Monitor;
  mMonitor->ignoreSession( Akonadi::Session::defaultSession() );

  connect( mMonitor, SIGNAL( itemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) ),
           mParent, SLOT( itemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) ) );
}


ContactEditor::ContactEditor( Mode mode, AbstractContactEditorWidget *editorWidget, QWidget *parent )
  : QWidget( parent ), d( new Private( this ) )
{
  d->mMode = mode;
  d->mEditorWidget = editorWidget;

  QVBoxLayout *layout = new QVBoxLayout( this );
  layout->setMargin( 0 );
  layout->setSpacing( 0 );
  layout->addWidget( d->mEditorWidget );
}

ContactEditor::~ContactEditor()
{
  delete d;
}

void ContactEditor::loadContact( const Akonadi::Item &item )
{
  if ( d->mMode == CreateMode )
    Q_ASSERT_X( false, "ContactEditor::loadContact", "You are calling loadContact in CreateMode!" );

  Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( item );
  job->fetchScope().fetchFullPayload();
  job->fetchScope().fetchAttribute<ContactMetaDataAttribute>();
  job->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );

  connect( job, SIGNAL( result( KJob* ) ), SLOT( fetchDone( KJob* ) ) );

  d->setupMonitor();
  d->mMonitor->setItemMonitored( item );
}

void ContactEditor::saveContact()
{
  if ( d->mMode == EditMode ) {
    if ( !d->mItem.isValid() )
      return;

    if ( d->mReadOnly )
      return;

    KABC::Addressee addr = d->mItem.payload<KABC::Addressee>();

    d->storeContact( addr, d->mContactMetaData );

    d->mContactMetaData.store( d->mItem );

    d->mItem.setPayload<KABC::Addressee>( addr );

    Akonadi::ItemModifyJob *job = new Akonadi::ItemModifyJob( d->mItem );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( storeDone( KJob* ) ) );
  } else if ( d->mMode == CreateMode ) {
    if ( !d->mDefaultCollection.isValid() ) {
      AutoQPointer<AddressBookSelectionDialog> dlg = new AddressBookSelectionDialog( AddressBookSelectionDialog::ContactsOnly, this );
      if ( dlg->exec() == KDialog::Accepted )
        setDefaultCollection( dlg->selectedAddressBook() );
      else
        return; // FIXME: should go back to the editor instead of aborting
    }

    KABC::Addressee addr;
    d->storeContact( addr, d->mContactMetaData );

    Akonadi::Item item;
    item.setPayload<KABC::Addressee>( addr );
    item.setMimeType( KABC::Addressee::mimeType() );

    d->mContactMetaData.store( item );

    Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob( item, d->mDefaultCollection );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( storeDone( KJob* ) ) );
  }
}

void ContactEditor::setDefaultCollection( const Akonadi::Collection &collection )
{
  d->mDefaultCollection = collection;
}

#include "contacteditor.moc"
