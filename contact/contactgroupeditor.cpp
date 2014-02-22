/*
    This file is part of Akonadi Contact.

    Copyright (c) 2007-2009 Tobias Koenig <tokoe@kde.org>

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

#include "contactgroupeditor.h"
#include "contactgroupeditor_p.h"

#include "autoqpointer_p.h"
#include "contactgroupmodel_p.h"
#include "contactgroupeditordelegate_p.h"
#include "waitingoverlay_p.h"

#include <akonadi/collectiondialog.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>
#include <kabc/contactgroup.h>
#include <klocalizedstring.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <KColorScheme>

#include <QtCore/QTimer>
#include <QMessageBox>

using namespace Akonadi;

ContactGroupEditor::Private::Private( ContactGroupEditor *parent )
  : mParent( parent ), mMonitor( 0 ), mReadOnly( false ), mGroupModel( 0 )
{
}

ContactGroupEditor::Private::~Private()
{
  delete mMonitor;
}

void ContactGroupEditor::Private::adaptHeaderSizes()
{
  mGui.membersView->header()->setDefaultSectionSize( mGui.membersView->header()->width() / 2 );
  mGui.membersView->header()->resizeSections( QHeaderView::Interactive );
}

void ContactGroupEditor::Private::itemFetchDone( KJob *job )
{
  if ( job->error() ) {
    return;
  }

  ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob*>( job );
  if ( !fetchJob ) {
    return;
  }

  if ( fetchJob->items().isEmpty() ) {
    return;
  }

  mItem = fetchJob->items().first();

  mReadOnly = false;
  if ( mMode == ContactGroupEditor::EditMode ) {
    // if in edit mode we have to fetch the parent collection to find out
    // about the modify rights of the item

    Akonadi::CollectionFetchJob *collectionFetchJob = new Akonadi::CollectionFetchJob( mItem.parentCollection(),
                                                                                       Akonadi::CollectionFetchJob::Base );
    mParent->connect( collectionFetchJob, SIGNAL(result(KJob*)),
                      SLOT(parentCollectionFetchDone(KJob*)) );
  } else {
    const KABC::ContactGroup group = mItem.payload<KABC::ContactGroup>();
    loadContactGroup( group );

    setReadOnly( mReadOnly );

    QTimer::singleShot( 0, mParent, SLOT(adaptHeaderSizes()) );
  }
}

void ContactGroupEditor::Private::parentCollectionFetchDone( KJob *job )
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

  const KABC::ContactGroup group = mItem.payload<KABC::ContactGroup>();
  loadContactGroup( group );

  setReadOnly( mReadOnly );

  QTimer::singleShot( 0, mParent, SLOT(adaptHeaderSizes()) );
}

void ContactGroupEditor::Private::storeDone( KJob *job )
{
  if ( job->error() ) {
    emit mParent->error( job->errorString() );
    return;
  }

  if ( mMode == EditMode ) {
    emit mParent->contactGroupStored( mItem );
  } else if ( mMode == CreateMode ) {
    emit mParent->contactGroupStored( static_cast<ItemCreateJob*>( job )->item() );
  }
}

void ContactGroupEditor::Private::itemChanged( const Item&, const QSet<QByteArray>& )
{
  AutoQPointer<QMessageBox> dlg = new QMessageBox( mParent ); //krazy:exclude=qclasses

  dlg->setInformativeText( i18n( "The contact group has been changed by someone else.\nWhat should be done?" ) );
  dlg->addButton( i18n( "Take over changes" ), QMessageBox::AcceptRole );
  dlg->addButton( i18n( "Ignore and Overwrite changes" ), QMessageBox::RejectRole );

  if ( dlg->exec() == QMessageBox::AcceptRole ) {
    ItemFetchJob *job = new ItemFetchJob( mItem );
    job->fetchScope().fetchFullPayload();
    job->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );

    mParent->connect( job, SIGNAL(result(KJob*)), mParent, SLOT(itemFetchDone(KJob*)) );
    new WaitingOverlay( job, mParent );
  }
}

void ContactGroupEditor::Private::loadContactGroup( const KABC::ContactGroup &group )
{
  mGui.groupName->setText( group.name() );

  mGroupModel->loadContactGroup( group );

  const QAbstractItemModel *model = mGui.membersView->model();
  mGui.membersView->setCurrentIndex( model->index( model->rowCount() - 1, 0 ) );

  if ( mMode == EditMode ) {
    mGui.membersView->setFocus();
  }

  mGui.membersView->header()->resizeSections( QHeaderView::Stretch );
}

bool ContactGroupEditor::Private::storeContactGroup( KABC::ContactGroup &group )
{
  if ( mGui.groupName->text().isEmpty() ) {
    KMessageBox::error( mParent, i18n( "The name of the contact group must not be empty." ) );
    return false;
  }

  group.setName( mGui.groupName->text() );

  if ( !mGroupModel->storeContactGroup( group ) ) {
    KMessageBox::error( mParent, mGroupModel->lastErrorMessage() );
    return false;
  }

  return true;
}

void ContactGroupEditor::Private::setupMonitor()
{
  delete mMonitor;
  mMonitor = new Monitor;
  mMonitor->ignoreSession( Session::defaultSession() );

  connect( mMonitor, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)),
           mParent, SLOT(itemChanged(Akonadi::Item,QSet<QByteArray>)) );
}

void ContactGroupEditor::Private::setReadOnly( bool readOnly )
{
  mGui.groupName->setReadOnly( readOnly );
  mGui.membersView->setEnabled( !readOnly );
}

ContactGroupEditor::ContactGroupEditor( Mode mode, QWidget *parent )
  : QWidget( parent ), d( new Private( this ) )
{
  d->mMode = mode;
  d->mGui.setupUi( this );

  d->mGui.membersView->setEditTriggers( QAbstractItemView::AllEditTriggers );

  d->mGroupModel = new ContactGroupModel( this );
  d->mGui.membersView->setModel( d->mGroupModel );
  d->mGui.membersView->setItemDelegate( new ContactGroupEditorDelegate( d->mGui.membersView, this ) );

  if ( mode == CreateMode ) {
    KABC::ContactGroup dummyGroup;
    d->mGroupModel->loadContactGroup( dummyGroup );

    QTimer::singleShot( 0, this, SLOT(adaptHeaderSizes()) );
    QTimer::singleShot( 0, d->mGui.groupName, SLOT(setFocus()) );
  }

  d->mGui.membersView->header()->setStretchLastSection( true );
}

ContactGroupEditor::~ContactGroupEditor()
{
   delete d;
}

void ContactGroupEditor::loadContactGroup( const Akonadi::Item &item )
{
  if ( d->mMode == CreateMode ) {
    Q_ASSERT_X( false, "ContactGroupEditor::loadContactGroup", "You are calling loadContactGroup in CreateMode!" );
  }

  ItemFetchJob *job = new ItemFetchJob( item );
  job->fetchScope().fetchFullPayload();
  job->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );

  connect( job, SIGNAL(result(KJob*)), SLOT(itemFetchDone(KJob*)) );

  d->setupMonitor();
  d->mMonitor->setItemMonitored( item );

  new WaitingOverlay( job, this );
}

bool ContactGroupEditor::saveContactGroup()
{
  if ( d->mMode == EditMode ) {
    if ( !d->mItem.isValid() ) {
      return false;
    }

    if ( d->mReadOnly ) {
      return true;
    }

    KABC::ContactGroup group = d->mItem.payload<KABC::ContactGroup>();

    if ( !d->storeContactGroup( group ) ) {
      return false;
    }

    d->mItem.setPayload<KABC::ContactGroup>( group );

    ItemModifyJob *job = new ItemModifyJob( d->mItem );
    connect( job, SIGNAL(result(KJob*)), SLOT(storeDone(KJob*)) );
  } else if ( d->mMode == CreateMode ) {
    if ( !d->mDefaultCollection.isValid() ) {
      const QStringList mimeTypeFilter( KABC::ContactGroup::mimeType() );

      AutoQPointer<CollectionDialog> dlg = new CollectionDialog( this );
      dlg->setMimeTypeFilter( mimeTypeFilter );
      dlg->setAccessRightsFilter( Collection::CanCreateItem );
      dlg->setCaption( i18n( "Select Address Book" ) );
      dlg->setDescription( i18n( "Select the address book the new contact group shall be saved in:" ) );

      if ( dlg->exec() == KDialog::Accepted ) {
        setDefaultAddressBook( dlg->selectedCollection() );
      } else {
        return false;
      }
    }

    KABC::ContactGroup group;
    if ( !d->storeContactGroup( group ) ) {
      return false;
    }

    Item item;
    item.setPayload<KABC::ContactGroup>( group );
    item.setMimeType( KABC::ContactGroup::mimeType() );

    ItemCreateJob *job = new ItemCreateJob( item, d->mDefaultCollection );
    connect( job, SIGNAL(result(KJob*)), SLOT(storeDone(KJob*)) );
  }

  return true;
}

void ContactGroupEditor::setContactGroupTemplate( const KABC::ContactGroup &group )
{
  d->mGroupModel->loadContactGroup( group );
  d->mGui.membersView->header()->setDefaultSectionSize( d->mGui.membersView->header()->width() / 2 );
  d->mGui.membersView->header()->resizeSections( QHeaderView::Interactive );
}

void ContactGroupEditor::setDefaultAddressBook( const Akonadi::Collection &collection )
{
  d->mDefaultCollection = collection;
}

void ContactGroupEditor::groupNameIsValid(bool isValid)
{
#ifndef QT_NO_STYLE_STYLESHEET
  QString styleSheet;
  if ( !isValid ) {
    const KColorScheme::BackgroundRole bgColorScheme( KColorScheme::NegativeBackground );
    KStatefulBrush bgBrush( KColorScheme::View, bgColorScheme );
    styleSheet = QString::fromLatin1( "QLineEdit{ background-color:%1 }" ).
           arg( bgBrush.brush( this ).color().name() );
  }
  d->mGui.groupName->setStyleSheet( styleSheet );
#endif
}

#include "moc_contactgroupeditor.cpp"
