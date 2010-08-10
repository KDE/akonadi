/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 - 2010 Tobias Koenig <tokoe@kde.org>

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

#include "standardcontactactionmanager.h"

#include <akonadi/agentfilterproxymodel.h>
#include <akonadi/agentinstance.h>
#include <akonadi/agentinstancecreatejob.h>
#include <akonadi/agentmanager.h>
#include <akonadi/agenttypedialog.h>
#include <akonadi/entitytreemodel.h>
#include <kabc/addressee.h>
#include <kabc/contactgroup.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <QtCore/QPointer>
#include <QtGui/QItemSelectionModel>

using namespace Akonadi;

class StandardContactActionManager::Private
{
  public:
    Private( KActionCollection *actionCollection, QWidget *parentWidget, StandardContactActionManager *parent )
      : mActionCollection( actionCollection ), mParentWidget( parentWidget ),
        mCollectionSelectionModel( 0 ), mItemSelectionModel( 0 ), mParent( parent )
    {
      mGenericManager = new StandardActionManager( actionCollection, parentWidget );
      mParent->connect( mGenericManager, SIGNAL( actionStateUpdated() ),
                        mParent, SIGNAL( actionStateUpdated() ) );
      mGenericManager->createAllActions();

      mGenericManager->action( Akonadi::StandardActionManager::CreateCollection )->setText( i18n( "Add Address Book Folder..." ) );
      mGenericManager->action( Akonadi::StandardActionManager::CreateCollection )->setWhatsThis( i18n( "Add a new address book folder to the currently selected address book folder." ) );
      mGenericManager->setActionText( Akonadi::StandardActionManager::CopyCollections, ki18np( "Copy Address Book Folder", "Copy %1 Address Book Folders" ) );
      mGenericManager->action( Akonadi::StandardActionManager::CopyCollections )->setWhatsThis( i18n( "Copy the selected address book folders to the clipboard." ) );
      mGenericManager->action( Akonadi::StandardActionManager::DeleteCollections )->setText( i18n( "Delete Address Book Folder" ) );
      mGenericManager->action( Akonadi::StandardActionManager::DeleteCollections )->setWhatsThis( i18n( "Delete the selected address book folders from the address book." ) );
      mGenericManager->action( Akonadi::StandardActionManager::SynchronizeCollections )->setText( i18n( "Update Address Book Folder" ) );
      mGenericManager->action( Akonadi::StandardActionManager::SynchronizeCollections )->setWhatsThis( i18n( "Update the content of the address book folder" ) );
      mGenericManager->setActionText( Akonadi::StandardActionManager::CutCollections, ki18np( "Cut Address Book Folder", "Cut %1 Address Book Folders" ) );
      mGenericManager->action( Akonadi::StandardActionManager::CutCollections )->setWhatsThis( i18n( "Cut the selected address book folders from the address book." ) );
      mGenericManager->action( Akonadi::StandardActionManager::CollectionProperties )->setText( i18n( "Folder Properties..." ) );
      mGenericManager->action( Akonadi::StandardActionManager::CollectionProperties)->setWhatsThis( i18n( "Open a dialog to edit the properties of the selected address book folder." ) );
      mGenericManager->setActionText( Akonadi::StandardActionManager::CopyItems, ki18np( "Copy Contact", "Copy %1 Contacts" ) );
      mGenericManager->action( Akonadi::StandardActionManager::CopyItems )->setWhatsThis( i18n( "Copy the selected contacts to the clipboard." ) );
      mGenericManager->setActionText( Akonadi::StandardActionManager::DeleteItems, ki18np( "Delete Contact", "Delete %1 Contacts" ) );
      mGenericManager->action( Akonadi::StandardActionManager::DeleteItems )->setWhatsThis( i18n( "Delete the selected contacts from the address book." ) );
      mGenericManager->setActionText( Akonadi::StandardActionManager::CutItems, ki18np( "Cut Contact", "Cut %1 Contacts" ) );
      mGenericManager->action( Akonadi::StandardActionManager::CutItems )->setWhatsThis( i18n( "Cut the selected contacts from the address book." ) );
    }

    ~Private()
    {
      delete mGenericManager;
    }

    static bool hasWritableCollection( const QModelIndex &index, const QString &mimeType )
    {
      const Akonadi::Collection collection = index.data( Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
      if ( collection.isValid() ) {
        if ( collection.contentMimeTypes().contains( mimeType ) && (collection.rights() & Akonadi::Collection::CanCreateItem) )
          return true;
      }

      const QAbstractItemModel *model = index.model();
      if ( !model )
        return false;

      for ( int row = 0; row < model->rowCount( index ); ++row ) {
        if ( hasWritableCollection( model->index( row, 0, index ), mimeType ) )
          return true;
      }

      return false;
    }

    bool hasWritableCollection( const QString &mimeType ) const
    {
      if ( !mCollectionSelectionModel )
        return false;

      const QAbstractItemModel *collectionModel = mCollectionSelectionModel->model();
      for ( int row = 0; row < collectionModel->rowCount(); ++row ) {
        if ( hasWritableCollection( collectionModel->index( row, 0, QModelIndex() ), mimeType ) )
          return true;
      }

      return false;
    }

    void updateActions()
    {
      int itemCount = 0;
      if ( mItemSelectionModel ) {
        itemCount = mItemSelectionModel->selectedRows().count();
        if ( itemCount == 1 ) {
          const QModelIndex index = mItemSelectionModel->selectedRows().first();
          if ( index.isValid() ) {
            const QString mimeType = index.data( EntityTreeModel::MimeTypeRole ).toString();
            if ( mimeType == KABC::Addressee::mimeType() ) {
              mGenericManager->setActionText( Akonadi::StandardActionManager::CopyItems,
                                              ki18np( "Copy Contact", "Copy %1 Contacts" ) );
              mGenericManager->setActionText( Akonadi::StandardActionManager::DeleteItems,
                                              ki18np( "Delete Contact", "Delete %1 Contacts" ) );
              mGenericManager->setActionText( Akonadi::StandardActionManager::CutItems,
                                              ki18np( "Cut Contact", "Cut %1 Contacts" ) );
              if ( mActions.contains( StandardContactActionManager::EditItem ) )
                mActions.value( StandardContactActionManager::EditItem )->setText( i18n( "Edit Contact..." ) );
            } else if ( mimeType == KABC::ContactGroup::mimeType() ) {
              mGenericManager->setActionText( Akonadi::StandardActionManager::CopyItems,
                                              ki18np( "Copy Group", "Copy %1 Groups" ) );
              mGenericManager->setActionText( Akonadi::StandardActionManager::DeleteItems,
                                              ki18np( "Delete Group", "Delete %1 Groups" ) );
              mGenericManager->setActionText( Akonadi::StandardActionManager::CutItems,
                                              ki18np( "Cut Group", "Cut %1 Groups" ) );
              if ( mActions.contains( StandardContactActionManager::EditItem ) )
                mActions.value( StandardContactActionManager::EditItem )->setText( i18n( "Edit Group..." ) );
            }
          }
        }
      }

      bool enableAddressBookActions = false;
      if ( mCollectionSelectionModel ) {
        if ( mCollectionSelectionModel->selectedRows().count() == 1 ) {
          const QModelIndex index = mCollectionSelectionModel->selectedRows().first();
          if ( index.isValid() ) {
            const Collection collection = index.data( EntityTreeModel::CollectionRole ).value<Collection>();
            if ( collection.isValid() ) {
              // actions are only enabled if the collection is a resource collection
              enableAddressBookActions = (collection.parentCollection() == Collection::root());
            }
          }
        }
      }

      if ( mActions.contains( StandardContactActionManager::DeleteAddressBook ) )
        mActions[ StandardContactActionManager::DeleteAddressBook ]->setEnabled( enableAddressBookActions );
      if ( mActions.contains( StandardContactActionManager::ConfigureAddressBook ) )
        mActions[ StandardContactActionManager::ConfigureAddressBook ]->setEnabled( enableAddressBookActions );

      if ( mActions.contains( StandardContactActionManager::CreateContact ) )
        mActions[ StandardContactActionManager::CreateContact ]->setEnabled( hasWritableCollection( KABC::Addressee::mimeType() ) );
      if ( mActions.contains( StandardContactActionManager::CreateContactGroup ) )
        mActions[ StandardContactActionManager::CreateContactGroup ]->setEnabled( hasWritableCollection( KABC::ContactGroup::mimeType() ) );

      if ( mActions.contains( StandardContactActionManager::EditItem ) ) {
        bool canEditItem = true;

        // only one selected item can be edited
        canEditItem = canEditItem && (itemCount == 1);

        // check whether parent collection allows changing the item
        const QModelIndexList rows = mItemSelectionModel->selectedRows();
        if ( rows.count() == 1 ) {
          const QModelIndex index = rows.first();
          const Collection parentCollection = index.data( EntityTreeModel::ParentCollectionRole ).value<Collection>();
          if ( parentCollection.isValid() )
            canEditItem = canEditItem && (parentCollection.rights() & Collection::CanChangeItem);
        }

        mActions.value( StandardContactActionManager::EditItem )->setEnabled( canEditItem );
      }

      emit mParent->actionStateUpdated();
    }

    void editTriggered()
    {
      if ( !mItemSelectionModel )
        return;

      if ( mItemSelectionModel->selectedIndexes().isEmpty() )
        return;

      const QModelIndex index = mItemSelectionModel->selectedIndexes().first();
      if ( !index.isValid() )
        return;

      const Item item = index.data( EntityTreeModel::ItemRole ).value<Item>();
      if ( !item.isValid() )
        return;

      emit mParent->editItem( item );
    }

    void addAddressBookTriggered()
    {
      QPointer<AgentTypeDialog> dlg = new AgentTypeDialog( mParentWidget );
      dlg->setWindowTitle( i18n( "Add Address Book" ) );
      dlg->agentFilterProxyModel()->addMimeTypeFilter( KABC::Addressee::mimeType() );
      dlg->agentFilterProxyModel()->addMimeTypeFilter( KABC::ContactGroup::mimeType() );
      dlg->agentFilterProxyModel()->addCapabilityFilter( QLatin1String( "Resource" ) ); // show only resources, no agents

      if ( dlg->exec() && dlg ) {
        const AgentType agentType = dlg->agentType();

        if ( agentType.isValid() ) {
          AgentInstanceCreateJob *job = new AgentInstanceCreateJob( agentType );
          job->configure( mParentWidget );
          mParent->connect( job, SIGNAL( result( KJob* ) ), mParent, SLOT( addAddressBookResult( KJob* ) ) );
          job->start();
        }
      }

      delete dlg;
    }

    void addAddressBookResult( KJob *job )
    {
      if ( job->error() ) {
        KMessageBox::error( mParentWidget, i18n( "Could not add address book: %1", job->errorString() ),
                            i18n( "Adding Address Book failed" ) );
      }
    }

    void deleteAddressBookTriggered()
    {
      if ( !mCollectionSelectionModel )
        return;

      if ( mCollectionSelectionModel->selectedIndexes().isEmpty() )
        return;

      const QModelIndex index = mCollectionSelectionModel->selectedIndexes().first();
      if ( !index.isValid() )
        return;

      const Collection collection = index.data( EntityTreeModel::CollectionRole).value<Collection>();
      const QString identifier = collection.resource();

      const AgentInstance instance = AgentManager::self()->instance( identifier );
      if ( !instance.isValid() )
        return;

      const QString text = i18n( "Do you really want to delete address book '%1'?", instance.name() );

      if ( KMessageBox::questionYesNo( mParentWidget, text,
           i18n( "Delete Address Book?"), KStandardGuiItem::del(), KStandardGuiItem::cancel(),
           QString(), KMessageBox::Dangerous ) != KMessageBox::Yes )
        return;

      AgentManager::self()->removeInstance( instance );
    }

    void configureAddressBookTriggered()
    {
      if ( !mCollectionSelectionModel )
        return;

      if ( mCollectionSelectionModel->selectedIndexes().isEmpty() )
        return;

      const QModelIndex index = mCollectionSelectionModel->selectedIndexes().first();
      if ( !index.isValid() )
        return;

      const Collection collection = index.data( EntityTreeModel::CollectionRole).value<Collection>();
      if ( !collection.isValid() )
        return;

      const QString identifier = collection.resource();

      AgentInstance instance = AgentManager::self()->instance( identifier );
      if ( !instance.isValid() )
        return;

      instance.configure();
    }

    KActionCollection *mActionCollection;
    QWidget *mParentWidget;
    StandardActionManager *mGenericManager;
    QItemSelectionModel *mCollectionSelectionModel;
    QItemSelectionModel *mItemSelectionModel;
    QHash<StandardContactActionManager::Type, KAction*> mActions;
    StandardContactActionManager *mParent;
};

StandardContactActionManager::StandardContactActionManager( KActionCollection *actionCollection, QWidget *parent )
  : QObject( parent ), d( new Private( actionCollection, parent, this ) )
{
}

StandardContactActionManager::~StandardContactActionManager()
{
  delete d;
}

void StandardContactActionManager::setCollectionSelectionModel( QItemSelectionModel *selectionModel )
{
  d->mCollectionSelectionModel = selectionModel;
  d->mGenericManager->setCollectionSelectionModel( selectionModel );

  connect( selectionModel->model(), SIGNAL( rowsInserted( const QModelIndex&, int, int ) ),
           SLOT( updateActions() ) );
  connect( selectionModel->model(), SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ),
           SLOT( updateActions() ) );
  connect( selectionModel, SIGNAL( selectionChanged( const QItemSelection&, const QItemSelection& ) ),
           SLOT( updateActions() ) );

  d->updateActions();
}

void StandardContactActionManager::setItemSelectionModel( QItemSelectionModel* selectionModel )
{
  d->mItemSelectionModel = selectionModel;
  d->mGenericManager->setItemSelectionModel( selectionModel );

  connect( selectionModel, SIGNAL( selectionChanged( const QItemSelection&, const QItemSelection& ) ),
           SLOT( updateActions() ) );

  d->updateActions();
}

KAction* StandardContactActionManager::createAction( Type type )
{
  Q_ASSERT( type >= CreateContact && type < LastType );

  if ( d->mActions.contains( type ) )
    return d->mActions.value( type );

  KAction *action = 0;

  switch ( type ) {
    case CreateContact:
      action = new KAction( d->mParentWidget );
      action->setIcon( KIcon( QLatin1String( "contact-new" ) ) );
      action->setText( i18n( "New &Contact..." ) );
      action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_N ) );
      action->setWhatsThis( i18n( "Create a new contact<p>You will be presented with a dialog where you can add data about a person, including addresses and phone numbers.</p>" ) );
      d->mActions.insert( CreateContact, action );
      d->mActionCollection->addAction( QString::fromLatin1( "akonadi_contact_create" ), action );
      break;
    case CreateContactGroup:
      action = new KAction( d->mParentWidget );
      action->setIcon( KIcon( QLatin1String( "user-group-new" ) ) );
      action->setText( i18n( "New &Group..." ) );
      action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_G ) );
      action->setWhatsThis( i18n( "Create a new group<p>You will be presented with a dialog where you can add a new group of contacts.</p>" ) );
      d->mActions.insert( CreateContactGroup, action );
      d->mActionCollection->addAction( QString::fromLatin1( "akonadi_contact_group_create" ), action );
      break;
    case EditItem:
      action = new KAction( d->mParentWidget );
      action->setIcon( KIcon( QLatin1String( "document-edit" ) ) );
      action->setText( i18n( "Edit Contact..." ) );
      action->setWhatsThis( i18n( "Edit the selected contact<p>You will be presented with a dialog where you can edit the data stored about a person, including addresses and phone numbers.</p>" ) );
      action->setEnabled( false );
      d->mActions.insert( EditItem, action );
      d->mActionCollection->addAction( QString::fromLatin1( "akonadi_contact_item_edit" ), action );
      connect( action, SIGNAL( triggered( bool ) ), this, SLOT( editTriggered() ) );
      break;
    case CreateAddressBook:
      action = new KAction( d->mParentWidget );
      action->setIcon( KIcon( QLatin1String( "folder-new" ) ) );
      action->setText( i18n( "Add &Address Book..." ) );
      action->setWhatsThis( i18n( "Add a new address book<p>You will be presented with a dialog where you can select the type of the address book that shall be added.</p>" ) );
      d->mActions.insert( CreateAddressBook, action );
      d->mActionCollection->addAction( QString::fromLatin1( "akonadi_addressbook_create" ), action );
      connect( action, SIGNAL( triggered( bool ) ), this, SLOT( addAddressBookTriggered() ) );
      break;
    case DeleteAddressBook:
      action = new KAction( d->mParentWidget );
      action->setIcon( KIcon( QLatin1String( "edit-delete" ) ) );
      action->setText( i18n( "&Delete Address Book" ) );
      action->setWhatsThis( i18n( "Delete the selected address book<p>The currently selected address book will be deleted, along with all the contacts and contact groups it contains.</p>" ) );
      action->setEnabled( false );
      d->mActions.insert( DeleteAddressBook, action );
      d->mActionCollection->addAction( QString::fromLatin1( "akonadi_addressbook_delete" ), action );
      connect( action, SIGNAL( triggered( bool ) ), this, SLOT( deleteAddressBookTriggered() ) );
      break;
    case ConfigureAddressBook:
      action = new KAction( d->mParentWidget );
      action->setIcon( KIcon( QLatin1String( "configure" ) ) );
      action->setText( i18n( "Address Book Properties..." ) );
      action->setWhatsThis( i18n( "Open a dialog to edit properties of the selected address book." ) );
      action->setEnabled( false );
      d->mActions.insert( ConfigureAddressBook, action );
      d->mActionCollection->addAction( QString::fromLatin1( "akonadi_addressbook_properties" ), action );
      connect( action, SIGNAL( triggered( bool ) ), this, SLOT( configureAddressBookTriggered() ) );
      break;
    default:
      Q_ASSERT( false ); // should never happen
      break;
  }

  return action;
}

void StandardContactActionManager::createAllActions()
{
  createAction( CreateContact );
  createAction( CreateContactGroup );
  createAction( EditItem );
  createAction( CreateAddressBook );
  createAction( DeleteAddressBook );
  createAction( ConfigureAddressBook );

  d->mGenericManager->createAllActions();

  d->updateActions();
}

KAction* StandardContactActionManager::action( Type type ) const
{
  if ( d->mActions.contains( type ) )
    return d->mActions.value( type );
  else
    return 0;
}

#include "standardcontactactionmanager.moc"
