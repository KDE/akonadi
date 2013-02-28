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

#include "abstractcontacteditorwidget_p.h"
#include "autoqpointer_p.h"
#include "contactmetadata_p.h"
#include "contactmetadataattribute_p.h"
#include "editor/contacteditorwidget.h"

#include <akonadi/collectiondialog.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>
#include <kabc/addressee.h>
#include <klocalizedstring.h>

#include <QtCore/QPointer>
#include <QVBoxLayout>
#include <QMessageBox>

using namespace Akonadi;

class ContactEditor::Private
{
  public:
    Private( ContactEditor::Mode mode, ContactEditor::DisplayMode displayMode, AbstractContactEditorWidget *editorWidget, ContactEditor *parent )
      : mParent( parent ), mMode( mode ), mMonitor( 0 ), mReadOnly( false )
    {
      if ( editorWidget ) {
        mEditorWidget = editorWidget;
#ifndef DISABLE_EDITOR_WIDGETS
      } else {
        mEditorWidget = new ContactEditorWidget(displayMode == FullMode ? ContactEditorWidget::FullMode : ContactEditorWidget::VCardMode, 0);
#endif
      }

      QVBoxLayout *layout = new QVBoxLayout( mParent );
      layout->setMargin( 0 );
      layout->setSpacing( 0 );
      layout->addWidget( mEditorWidget );
    }

    ~Private()
    {
      delete mMonitor;
    }

    void itemFetchDone( KJob* );
    void parentCollectionFetchDone( KJob* );
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

void ContactEditor::Private::itemFetchDone( KJob *job )
{
  if ( job->error() != KJob::NoError ) {
    return;
  }

  Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob*>( job );
  if ( !fetchJob ) {
    return;
  }

  if ( fetchJob->items().isEmpty() ) {
    return;
  }

  mItem = fetchJob->items().first();

  mReadOnly = false;
  if ( mMode == ContactEditor::EditMode ) {
    // if in edit mode we have to fetch the parent collection to find out
    // about the modify rights of the item

    Akonadi::CollectionFetchJob *collectionFetchJob = new Akonadi::CollectionFetchJob( mItem.parentCollection(),
                                                                                       Akonadi::CollectionFetchJob::Base );
    mParent->connect( collectionFetchJob, SIGNAL(result(KJob*)),
                      SLOT(parentCollectionFetchDone(KJob*)) );
  } else {
    const KABC::Addressee addr = mItem.payload<KABC::Addressee>();
    mContactMetaData.load( mItem );
    loadContact( addr, mContactMetaData );
    mEditorWidget->setReadOnly( mReadOnly );
  }
}

void ContactEditor::Private::parentCollectionFetchDone( KJob *job )
{
  if ( job->error() ) {
    return;
  }

  Akonadi::CollectionFetchJob *fetchJob = qobject_cast<Akonadi::CollectionFetchJob*>( job );
  if ( !fetchJob ) {
    return;
  }

  const Akonadi::Collection parentCollection = fetchJob->collections().first();
  if ( parentCollection.isValid() ) {
    mReadOnly = !( parentCollection.rights() & Collection::CanChangeItem );
  }

  mEditorWidget->setReadOnly( mReadOnly );

  const KABC::Addressee addr = mItem.payload<KABC::Addressee>();
  mContactMetaData.load( mItem );
  loadContact( addr, mContactMetaData );
}

void ContactEditor::Private::storeDone( KJob *job )
{
  if ( job->error() != KJob::NoError ) {
    emit mParent->error( job->errorString() );
    emit mParent->finished();
    return;
  }

  if ( mMode == EditMode ) {
    emit mParent->contactStored( mItem );
  } else if ( mMode == CreateMode ) {
    emit mParent->contactStored( static_cast<Akonadi::ItemCreateJob*>( job )->item() );
  }
  emit mParent->finished();
}

void ContactEditor::Private::itemChanged( const Akonadi::Item&, const QSet<QByteArray>& )
{
  QPointer<QMessageBox> dlg = new QMessageBox( mParent ); //krazy:exclude=qclasses

  dlg->setInformativeText( i18n( "The contact has been changed by someone else.\nWhat should be done?" ) );
  dlg->addButton( i18n( "Take over changes" ), QMessageBox::AcceptRole );
  dlg->addButton( i18n( "Ignore and Overwrite changes" ), QMessageBox::RejectRole );

  if ( dlg->exec() == QMessageBox::AcceptRole ) {
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( mItem );
    job->fetchScope().fetchFullPayload();
    job->fetchScope().fetchAttribute<ContactMetaDataAttribute>();
    job->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );

    mParent->connect( job, SIGNAL(result(KJob*)), mParent, SLOT(itemFetchDone(KJob*)) );
  }

  delete dlg;
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

  connect( mMonitor, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)),
           mParent, SLOT(itemChanged(Akonadi::Item,QSet<QByteArray>)) );
}


ContactEditor::ContactEditor( Mode mode, QWidget *parent )
  : QWidget( parent ), d( new Private( mode, FullMode, 0, this ) )
{
}

ContactEditor::ContactEditor( Mode mode, AbstractContactEditorWidget *editorWidget, QWidget *parent )
  : QWidget( parent ), d( new Private( mode, FullMode, editorWidget, this ) )
{
}

ContactEditor::ContactEditor( Mode mode, DisplayMode displayMode, QWidget *parent )
  : QWidget( parent ), d( new Private( mode, displayMode, 0, this ) )
{
}


ContactEditor::~ContactEditor()
{
  delete d;
}

void ContactEditor::loadContact( const Akonadi::Item &item )
{
  if ( d->mMode == CreateMode ) {
    Q_ASSERT_X( false, "ContactEditor::loadContact", "You are calling loadContact in CreateMode!" );
  }

  Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( item );
  job->fetchScope().fetchFullPayload();
  job->fetchScope().fetchAttribute<ContactMetaDataAttribute>();
  job->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );

  connect( job, SIGNAL(result(KJob*)), SLOT(itemFetchDone(KJob*)) );

  d->setupMonitor();
  d->mMonitor->setItemMonitored( item );
}

KABC::Addressee ContactEditor::contact()
{
  KABC::Addressee addr;
  d->storeContact( addr, d->mContactMetaData );
  return addr;
}

void ContactEditor::saveContactInAddressBook()
{
    if ( d->mMode == EditMode ) {
      if ( !d->mItem.isValid() || d->mReadOnly ) {
        emit finished();
        return;
      }

      KABC::Addressee addr = d->mItem.payload<KABC::Addressee>();

      d->storeContact( addr, d->mContactMetaData );

      d->mContactMetaData.store( d->mItem );

      d->mItem.setPayload<KABC::Addressee>( addr );

      Akonadi::ItemModifyJob *job = new Akonadi::ItemModifyJob( d->mItem );
      connect( job, SIGNAL(result(KJob*)), SLOT(storeDone(KJob*)) );
    } else if ( d->mMode == CreateMode ) {
      if ( !d->mDefaultCollection.isValid() ) {
        const QStringList mimeTypeFilter( KABC::Addressee::mimeType() );

        AutoQPointer<CollectionDialog> dlg = new CollectionDialog( this );
        dlg->setMimeTypeFilter( mimeTypeFilter );
        dlg->setAccessRightsFilter( Collection::CanCreateItem );
        dlg->setCaption( i18n( "Select Address Book" ) );
        dlg->setDescription( i18n( "Select the address book the new contact shall be saved in:" ) );
        if ( dlg->exec() == KDialog::Accepted ) {
          setDefaultAddressBook( dlg->selectedCollection() );
        } else {
          return;
        }
      }

      KABC::Addressee addr;
      d->storeContact( addr, d->mContactMetaData );

      Akonadi::Item item;
      item.setPayload<KABC::Addressee>( addr );
      item.setMimeType( KABC::Addressee::mimeType() );

      d->mContactMetaData.store( item );

      Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob( item, d->mDefaultCollection );
      connect( job, SIGNAL(result(KJob*)), SLOT(storeDone(KJob*)) );
    }
}

bool ContactEditor::saveContact()
{
  if ( d->mMode == EditMode ) {
    if ( !d->mItem.isValid() ) {
      return true;
    }

    if ( d->mReadOnly ) {
      return true;
    }

    KABC::Addressee addr = d->mItem.payload<KABC::Addressee>();

    d->storeContact( addr, d->mContactMetaData );

    d->mContactMetaData.store( d->mItem );

    d->mItem.setPayload<KABC::Addressee>( addr );

    Akonadi::ItemModifyJob *job = new Akonadi::ItemModifyJob( d->mItem );
    connect( job, SIGNAL(result(KJob*)), SLOT(storeDone(KJob*)) );
  } else if ( d->mMode == CreateMode ) {
    if ( !d->mDefaultCollection.isValid() ) {
      const QStringList mimeTypeFilter( KABC::Addressee::mimeType() );

      AutoQPointer<CollectionDialog> dlg = new CollectionDialog( this );
      dlg->setMimeTypeFilter( mimeTypeFilter );
      dlg->setAccessRightsFilter( Collection::CanCreateItem );
      dlg->setCaption( i18n( "Select Address Book" ) );
      dlg->setDescription( i18n( "Select the address book the new contact shall be saved in:" ) );
      if ( dlg->exec() == KDialog::Accepted ) {
        setDefaultAddressBook( dlg->selectedCollection() );
      } else {
        return false;
      }
    }

    KABC::Addressee addr;
    d->storeContact( addr, d->mContactMetaData );

    Akonadi::Item item;
    item.setPayload<KABC::Addressee>( addr );
    item.setMimeType( KABC::Addressee::mimeType() );

    d->mContactMetaData.store( item );

    Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob( item, d->mDefaultCollection );
    connect( job, SIGNAL(result(KJob*)), SLOT(storeDone(KJob*)) );
  }

  return true;
}

void ContactEditor::setContactTemplate( const KABC::Addressee &contact )
{
  d->loadContact( contact, d->mContactMetaData );
}

void ContactEditor::setDefaultAddressBook( const Akonadi::Collection &collection )
{
  d->mDefaultCollection = collection;
}

#include "moc_contacteditor.cpp"
