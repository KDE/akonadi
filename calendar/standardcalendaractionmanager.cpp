/*
 *  Copyright (C) 2010 Casey Link <unnamedrambler@gmail.com>
 *  Copyright (C) 2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
 *   Copyright (c) 2009 - 2010 Tobias Koenig <tokoe@kde.org>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */

#include "standardcalendaractionmanager.h"

#include <akonadi/agentinstance.h>
#include <akonadi/agentinstancecreatejob.h>
#include <akonadi/agentmanager.h>
#include <akonadi/entitytreemodel.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>

#include <QtCore/QPointer>
#include <QtGui/QItemSelectionModel>

using namespace Akonadi;

class StandardCalendarActionManager::Private
{
  public:
    Private( KActionCollection *actionCollection, QWidget *parentWidget, StandardCalendarActionManager *parent )
      : mActionCollection( actionCollection ),
        mParentWidget( parentWidget ),
        mCollectionSelectionModel( 0 ),
        mItemSelectionModel( 0 ),
        mParent( parent )
    {
      mGenericManager = new StandardActionManager( actionCollection, parentWidget );
      mParent->connect( mGenericManager, SIGNAL( actionStateUpdated() ),
                        mParent, SIGNAL( actionStateUpdated() ) );
      mGenericManager->createAllActions();

      mGenericManager->action( Akonadi::StandardActionManager::DeleteItems )->setText( i18n( "&Delete Events" ) );
      mGenericManager->action( Akonadi::StandardActionManager::DeleteItems )->setWhatsThis( i18n( "Delete the selected events." ) );
      mGenericManager->action( Akonadi::StandardActionManager::CreateCollection )->setText( i18n( "Add Calendar Folder..." ) );
      mGenericManager->action( Akonadi::StandardActionManager::CreateCollection )->setWhatsThis( i18n( "Add a new calendar folder to the currently selected calendar folder." ) );
      mGenericManager->setActionText( Akonadi::StandardActionManager::CopyCollections, ki18np( "Copy Calendar Folder", "Copy %1 Calendar Folders" ) );
      mGenericManager->action( Akonadi::StandardActionManager::CopyCollections )->setWhatsThis( i18n( "Copy the selected calendar folders to the clipboard." ) );
      mGenericManager->action( Akonadi::StandardActionManager::DeleteCollections )->setText( i18n( "Delete Calendar Folder" ) );
      mGenericManager->action( Akonadi::StandardActionManager::DeleteCollections )->setWhatsThis( i18n( "Delete the selected calendar folders from the calendar." ) );
      mGenericManager->action( Akonadi::StandardActionManager::SynchronizeCollections )->setText( i18n( "Update Calendar Folder" ) );
      mGenericManager->action( Akonadi::StandardActionManager::SynchronizeCollections )->setWhatsThis( i18n( "Update the content of the calendar folder" ) );
      mGenericManager->setActionText( Akonadi::StandardActionManager::CutCollections, ki18np( "Cut Calendar Folder", "Cut %1 Calendar Folders" ) );
      mGenericManager->action( Akonadi::StandardActionManager::CutCollections )->setWhatsThis( i18n( "Cut the selected calendar folders from the calendar." ) );
      mGenericManager->action( Akonadi::StandardActionManager::CollectionProperties )->setText( i18n( "Folder Properties..." ) );
      mGenericManager->action( Akonadi::StandardActionManager::CollectionProperties)->setWhatsThis( i18n( "Open a dialog to edit the properties of the selected calendar folder." ) );
      mGenericManager->setActionText( Akonadi::StandardActionManager::CopyItems, ki18np( "Copy Event", "Copy %1 Events" ) );
      mGenericManager->action( Akonadi::StandardActionManager::CopyItems )->setWhatsThis( i18n( "Copy the selected events to the clipboard." ) );
      mGenericManager->setActionText( Akonadi::StandardActionManager::DeleteItems, ki18np( "Delete Event", "Delete %1 Events" ) );
      mGenericManager->action( Akonadi::StandardActionManager::DeleteItems )->setWhatsThis( i18n( "Delete the selected events from the calendar." ) );
      mGenericManager->setActionText( Akonadi::StandardActionManager::CutItems, ki18np( "Cut Event", "Cut %1 Events" ) );
      mGenericManager->action( Akonadi::StandardActionManager::CutItems )->setWhatsThis( i18n( "Cut the selected events from the calendar." ) );
      mGenericManager->action( Akonadi::StandardActionManager::CreateResource )->setText( i18n( "Add &Calendar..." ) );
      mGenericManager->action( Akonadi::StandardActionManager::CreateResource )->setWhatsThis( i18n( "Add a new calendar<p>You will be presented with a dialog where you can select the type of the calendar that shall be added.</p>" ) );
      mGenericManager->action( Akonadi::StandardActionManager::DeleteResource )->setText( i18n( "&Delete Calendar" ) );
      mGenericManager->action( Akonadi::StandardActionManager::DeleteResource )->setWhatsThis( i18n( "Delete the selected calendar<p>The currently selected calendar will be deleted, along with all the events, todos and journals it contains.</p>" ) );
      mGenericManager->action( Akonadi::StandardActionManager::ResourceProperties )->setText( i18n( "Calendar Properties..." ) );
      mGenericManager->action( Akonadi::StandardActionManager::ResourceProperties )->setWhatsThis( i18n( "Open a dialog to edit properties of the selected calendar." ) );
      mGenericManager->action( Akonadi::StandardActionManager::SynchronizeResource )->setText( i18n( "Update Calendar" ) );
      mGenericManager->action( Akonadi::StandardActionManager::SynchronizeResource )->setWhatsThis( i18n( "Updates the content of all folders of the calendar." ) );
      mGenericManager->action( Akonadi::StandardActionManager::CopyItemToMenu )->setText( i18n( "&Copy to Calendar" ) );
      mGenericManager->action( Akonadi::StandardActionManager::CopyItemToMenu )->setWhatsThis( i18n( "Copy the selected event to a different calendar." ) );
      mGenericManager->action( Akonadi::StandardActionManager::MoveItemToMenu )->setText( i18n( "&Move to Calendar" ) );
      mGenericManager->action( Akonadi::StandardActionManager::MoveItemToMenu  )->setWhatsThis( i18n( "Move the selected event to a different calendar." ) );


      mGenericManager->setContextText( StandardActionManager::CreateCollection, StandardActionManager::DialogTitle,
                                       i18nc( "@title:window", "New Calendar Folder" ) );
      mGenericManager->setContextText( StandardActionManager::CreateCollection, StandardActionManager::ErrorMessageText,
                                       i18n( "Could not create calendar folder: %1" ) );
      mGenericManager->setContextText( StandardActionManager::CreateCollection, StandardActionManager::ErrorMessageTitle,
                                       i18n( "Calendar folder creation failed" ) );

      mGenericManager->setContextText( StandardActionManager::DeleteCollections, StandardActionManager::MessageBoxText,
                                       i18n( "Do you really want to delete calendar folder '%1' and all its sub-folders?" ) );
      mGenericManager->setContextText( StandardActionManager::DeleteCollections, StandardActionManager::MessageBoxAlternativeText,
                                       i18n( "Do you really want to delete the calendar search folder '%1'?" ) );
      mGenericManager->setContextText( StandardActionManager::DeleteCollections, StandardActionManager::MessageBoxTitle,
                                       i18nc( "@title:window", "Delete calendar folder?" ) );
      mGenericManager->setContextText( StandardActionManager::DeleteCollections, StandardActionManager::ErrorMessageText,
                                       i18n( "Could not delete calendar folder: %1" ) );
      mGenericManager->setContextText( StandardActionManager::DeleteCollections, StandardActionManager::ErrorMessageTitle,
                                       i18n( "Calendar folder deletion failed" ) );

      mGenericManager->setContextText( StandardActionManager::CollectionProperties, StandardActionManager::DialogTitle,
                                       i18nc( "@title:window", "Properties of Calendar Folder %1" ) );

      mGenericManager->setContextText( StandardActionManager::DeleteItems, StandardActionManager::MessageBoxText,
                                       i18n( "Do you really want to delete all selected events?" ) );
      mGenericManager->setContextText( StandardActionManager::DeleteItems, StandardActionManager::MessageBoxTitle,
                                       i18nc( "@title:window", "Delete Events?" ) );

      mGenericManager->setContextText( StandardActionManager::CreateResource, StandardActionManager::DialogTitle,
                                       i18nc( "@title:window", "Add Calendar" ) );
      mGenericManager->setContextText( StandardActionManager::CreateResource, StandardActionManager::ErrorMessageText,
                                       i18n( "Could not create calendar: %1" ) );
      mGenericManager->setContextText( StandardActionManager::CreateResource, StandardActionManager::ErrorMessageTitle,
                                       i18n( "Calendar creation failed" ) );

      mGenericManager->setContextText( StandardActionManager::DeleteResource, StandardActionManager::MessageBoxText,
                                       i18n( "Do you really want to delete calendar '%1'?" ) );
      mGenericManager->setContextText( StandardActionManager::DeleteResource, StandardActionManager::MessageBoxTitle,
                                       i18nc( "@title:window", "Delete Calendar?" ) );

      mGenericManager->setContextText( StandardActionManager::Paste, StandardActionManager::ErrorMessageText,
                                       i18n( "Could not paste event: %1" ) );
      mGenericManager->setContextText( StandardActionManager::Paste, StandardActionManager::ErrorMessageTitle,
                                       i18n( "Paste failed" ) );

      mGenericManager->setMimeTypeFilter( QStringList() << QLatin1String( "text/calendar" ) );
      mGenericManager->setCapabilityFilter( QStringList() << QLatin1String( "Resource" ) );
    }

    ~Private()
    {
      delete mGenericManager;
    }

    static bool hasWritableCollection( const QModelIndex &index, const QString &mimeType )
    {
      const Akonadi::Collection collection = index.data( Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
      if ( collection.isValid() ) {
        if ( collection.contentMimeTypes().contains( mimeType ) && ( collection.rights() & Akonadi::Collection::CanCreateItem ) )
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
      //TODO impl
    }

    Collection selectedCollection() const
    {
      if ( !mCollectionSelectionModel )
        return Collection();

      if ( mCollectionSelectionModel->selectedIndexes().isEmpty() )
        return Collection();

      const QModelIndex index = mCollectionSelectionModel->selectedIndexes().first();
      if ( !index.isValid() )
        return Collection();

      return index.data( EntityTreeModel::CollectionRole ).value<Collection>();
    }

    AgentInstance selectedAgentInstance() const
    {
      const Collection collection = selectedCollection();
      if ( !collection.isValid() )
        return AgentInstance();

      const QString identifier = collection.resource();

      return AgentManager::self()->instance( identifier );
    }

    void slotCreateEvent()
    {
      if( mInterceptedActions.contains( StandardCalendarActionManager::CreateEvent ) )
        return;
    }

    void slotMakeDefault()
    {
      if ( mInterceptedActions.contains( StandardCalendarActionManager::DefaultMake ) )
        return;

      if( mItemSelectionModel->selection().indexes().isEmpty() )
        return;

      const QModelIndex index = mItemSelectionModel->selectedIndexes().first();
      if ( !index.isValid() )
        return;

      const Collection calendar = index.data( EntityTreeModel::CollectionRole ).value<Collection>();
      if ( !calendar.isValid() )
        return;

      //TODO impl
    }

    void slotRemoveDefault()
    {
      if ( mInterceptedActions.contains( StandardCalendarActionManager::DefaultMake ) )
        return;

      if( mItemSelectionModel->selection().indexes().isEmpty() )
        return;

      const QModelIndex index = mItemSelectionModel->selectedIndexes().first();
      if ( !index.isValid() )
        return;

      const Collection calendar = index.data( EntityTreeModel::CollectionRole ).value<Collection>();
      if ( !calendar.isValid() )
        return;

      //TODO impl
    }

    void slotStartMaintenanceMode()
    {
      //TODO impl
    }

    void slotAddFavorite()
    {
      //TODO impl
    }

    void slotRemoveFavorite()
    {
      //TODO impl
    }

    void slotRenameFavorite()
    {
      //TODO impl
    }

    void slotEditEvent()
    {
      //TODO impl
    }

    void slotPublishItemInformation()
    {
      //TODO impl
    }
    void slotSendInvitations()
    {
      //TODO impl
    }

    void slotSendStatusUpdate()
    {
      //TODO impl
    }

    void slotSendCancellation()
    {
      //TODO impl
    }

    void slotSendAsICal()
    {
      //TODO impl
    }

    void slotMailFreeBusy()
    {
      //TODO impl
    }

    void slotUploadFeeBusy()
    {
      //TODO impl
    }
    void slotSaveAllAttachments()
    {
      //TODO impl
    }

    KActionCollection *mActionCollection;
    QWidget *mParentWidget;
    StandardActionManager *mGenericManager;
    QItemSelectionModel *mCollectionSelectionModel;
    QItemSelectionModel *mItemSelectionModel;
    QHash<StandardCalendarActionManager::Type, KAction*> mActions;
    QSet<StandardCalendarActionManager::Type> mInterceptedActions;
    StandardCalendarActionManager *mParent;
};

Akonadi::StandardCalendarActionManager::StandardCalendarActionManager( KActionCollection* actionCollection, QWidget* parent )
  : QObject( parent ),
    d( new Private( actionCollection, parent, this ) )
{

}

StandardCalendarActionManager::~StandardCalendarActionManager()
{
  delete d;
}

void StandardCalendarActionManager::setCollectionSelectionModel( QItemSelectionModel* selectionModel )
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

void StandardCalendarActionManager::setItemSelectionModel( QItemSelectionModel* selectionModel )
{
  d->mItemSelectionModel = selectionModel;
  d->mGenericManager->setItemSelectionModel( selectionModel );

  connect( selectionModel, SIGNAL( selectionChanged( const QItemSelection&, const QItemSelection& ) ),
           SLOT( updateActions() ) );

  d->updateActions();
}

KAction* StandardCalendarActionManager::createAction( StandardCalendarActionManager::Type type )
{
  if ( d->mActions.contains( type ) )
    return d->mActions.value( type );

  KAction *action = 0;
  switch ( type ) {
    case CreateEvent:
      action = new KAction( d->mParentWidget );
      action->setIcon( KIcon( QLatin1String( "event-new" ) ) );
      action->setText( i18n( "New &Event..." ) );
//       action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_N ) );
      action->setWhatsThis( i18n( "Create a new event" ) );
      d->mActions.insert( CreateEvent, action );
      d->mActionCollection->addAction( QString::fromLatin1( "akonadi_event_create" ), action );
      connect( action, SIGNAL( triggered( bool ) ), this, SLOT( slotCreateEvent() ) );
      break;
    case DefaultRemove:
      action = new KAction( d->mParentWidget );
//       action->setIcon( KIcon( QLatin1String( "event-new" ) ) );
      action->setText( i18n( "&Remove Default" ) );
//       action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_N ) );
      action->setWhatsThis( i18n( "Remove the selected calendar as the default calendar. There can only be one default calendar at a time." ) );
      d->mActions.insert( DefaultRemove, action );
      d->mActionCollection->addAction( QString::fromLatin1( "akonadi_remove_default_calendar" ), action );
      connect( action, SIGNAL( triggered( bool ) ), this, SLOT( slotRemoveDefault()) );
      break;
    case DefaultMake:
      action = new KAction( d->mParentWidget );
      //       action->setIcon( KIcon( QLatin1String( "event-new" ) ) );
      action->setText( i18n( "&Make Default" ) );
      //       action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_N ) );
      action->setWhatsThis( i18n( "Make the selected calendar the default calendar, which will be loaded when the application starts. There can only be one default calendar at a time." ) );
      d->mActions.insert( DefaultMake, action );
      d->mActionCollection->addAction( QString::fromLatin1( "akonadi_make_default_calendar" ), action );
      connect( action, SIGNAL( triggered( bool ) ), this, SLOT( slotMakeDefault() ) );
      break;
    case StartMaintenanceMode:
      action = new KAction( d->mParentWidget );
      //       action->setIcon( KIcon( QLatin1String( "event-new" ) ) );
      action->setText( i18n( "&Start Maintenance Mode" ) );
      //       action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_N ) );
      action->setWhatsThis( i18n( "Opens a bulk actions dialog, that allows for mass move, copy, delete, and toggler reminders and alarms actions." ) );
      d->mActions.insert( StartMaintenanceMode, action );
      d->mActionCollection->addAction( QString::fromLatin1( "start_maintenance_mode" ), action );
      connect( action, SIGNAL( triggered( bool ) ), this, SLOT( slotStartMaintenanceMode() ) );
      break;
    case FavoriteAdd:
      action = new KAction( d->mParentWidget );
      //       action->setIcon( KIcon( QLatin1String( "event-new" ) ) );
      action->setText( i18n( "&Add Calendar to Favorites" ) );
      //       action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_N ) );
      action->setWhatsThis( i18n( "Adds the calendar to your list of favorites." ) );
      d->mActions.insert( FavoriteAdd, action );
      d->mActionCollection->addAction( QString::fromLatin1( "add_favorite" ), action );
      connect( action, SIGNAL( triggered( bool ) ), this, SLOT( slotAddFavorite() ) );
      break;
    case FavoriteRemove:
      action = new KAction( d->mParentWidget );
      //       action->setIcon( KIcon( QLatin1String( "event-new" ) ) );
      action->setText( i18n( "&Remove Calendar from Favorites" ) );
      //       action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_N ) );
      action->setWhatsThis( i18n( "Removes the calendar from your list of favorites." ) );
      d->mActions.insert( FavoriteRemove, action );
      d->mActionCollection->addAction( QString::fromLatin1( "remove_favorite" ), action );
      connect( action, SIGNAL( triggered( bool ) ), this, SLOT( slotRemoveFavorite() ) );
      break;
    case FavoriteRename:
      action = new KAction( d->mParentWidget );
      //       action->setIcon( KIcon( QLatin1String( "event-new" ) ) );
      action->setText( i18n( "&Rename Favorite" ) );
      //       action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_N ) );
      action->setWhatsThis( i18n( "Allows you to change the name of the favorite." ) );
      d->mActions.insert( FavoriteRename, action );
      d->mActionCollection->addAction( QString::fromLatin1( "rename_favorite" ), action );
      connect( action, SIGNAL( triggered( bool ) ), this, SLOT( slotRenameFavorite() ) );
      break;
    case EventEdit:
      action = new KAction( d->mParentWidget );
      //       action->setIcon( KIcon( QLatin1String( "event-new" ) ) );
      action->setText( i18n( "&Edit Event" ) );
      //       action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_N ) );
      action->setWhatsThis( i18n( "Edit the selected event." ) );
      d->mActions.insert( EventEdit, action );
      d->mActionCollection->addAction( QString::fromLatin1( "event_edit" ), action );
      connect( action, SIGNAL( triggered( bool ) ), this, SLOT( slotEditEvent() ) );
      break;
    case PublishItemInformation:
      action = new KAction( d->mParentWidget );
      //       action->setIcon( KIcon( QLatin1String( "event-new" ) ) );
      action->setText( i18n( "&Publish Item Information" ) );
      //       action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_N ) );
      action->setWhatsThis( i18n( "FIXME Publishes the items information." ) );
      d->mActions.insert( PublishItemInformation, action );
      d->mActionCollection->addAction( QString::fromLatin1( "publish_item_information" ), action );
      connect( action, SIGNAL( triggered( bool ) ), this, SLOT( slotPublishItemInformation() ) );
      break;
    case SendInvitations:
      action = new KAction( d->mParentWidget );
      //       action->setIcon( KIcon( QLatin1String( "event-new" ) ) );
      action->setText( i18n( "Send &Invitations" ) );
      //       action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_N ) );
      action->setWhatsThis( i18n( "FIXME Send Invitations" ) );
      d->mActions.insert( SendInvitations, action );
      d->mActionCollection->addAction( QString::fromLatin1( "send_invitations" ), action );
      connect( action, SIGNAL( triggered( bool ) ), this, SLOT( slotSendInvitations() ) );
      break;
    case SendStatusUpdate:
      action = new KAction( d->mParentWidget );
      //       action->setIcon( KIcon( QLatin1String( "event-new" ) ) );
      action->setText( i18n( "Send Status &Update" ) );
      //       action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_N ) );
      action->setWhatsThis( i18n( "FIXME Status Update" ) );
      d->mActions.insert( SendStatusUpdate, action );
      d->mActionCollection->addAction( QString::fromLatin1( "send_status_update" ), action );
      connect( action, SIGNAL( triggered( bool ) ), this, SLOT( slotSendStatusUpdate() ) );
      break;
    case SendCancellation:
      action = new KAction( d->mParentWidget );
      //       action->setIcon( KIcon( QLatin1String( "event-new" ) ) );
      action->setText( i18n( "Send &Cancellation" ) );
      //       action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_N ) );
      action->setWhatsThis( i18n( "FIXME Send Cancellation" ) );
      d->mActions.insert( SendCancellation, action );
      d->mActionCollection->addAction( QString::fromLatin1( "send_cancellation" ), action );
      connect( action, SIGNAL( triggered( bool ) ), this, SLOT( slotSendCancellation() ) );
      break;
    case SendAsICal:
      action = new KAction( d->mParentWidget );
      //       action->setIcon( KIcon( QLatin1String( "event-new" ) ) );
      action->setText( i18n( "Send as I&Cal" ) );
      //       action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_N ) );
      action->setWhatsThis( i18n( "FIXME Send As ICal" ) );
      d->mActions.insert( SendAsICal, action );
      d->mActionCollection->addAction( QString::fromLatin1( "send_as_ical" ), action );
      connect( action, SIGNAL( triggered( bool ) ), this, SLOT( slotSendAsICal() ) );
      break;
    case MailFreeBusy:
      action = new KAction( d->mParentWidget );
      //       action->setIcon( KIcon( QLatin1String( "event-new" ) ) );
      action->setText( i18n( "&Mail Free/Busy" ) );
      //       action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_N ) );
      action->setWhatsThis( i18n( "FIXME Email Free/Busy information" ) );
      d->mActions.insert( MailFreeBusy, action );
      d->mActionCollection->addAction( QString::fromLatin1( "mail_free_busy" ), action );
      connect( action, SIGNAL( triggered( bool ) ), this, SLOT( slotMailFreeBusy() ) );
      break;
    case UploadFeeBusy:
      action = new KAction( d->mParentWidget );
      //       action->setIcon( KIcon( QLatin1String( "event-new" ) ) );
      action->setText( i18n( "&Upload Free/Busy" ) );
      //       action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_N ) );
      action->setWhatsThis( i18n( "FIXME Upload Free/Busy information to the groupware server." ) );
      d->mActions.insert( UploadFeeBusy, action );
      d->mActionCollection->addAction( QString::fromLatin1( "upload_free_busy" ), action );
      connect( action, SIGNAL( triggered( bool ) ), this, SLOT( slotUploadFeeBusy() ) );
      break;
    case SaveAllAttachments:
      action = new KAction( d->mParentWidget );
      //       action->setIcon( KIcon( QLatin1String( "event-new" ) ) );
      action->setText( i18n( "&Save All Attachments" ) );
      //       action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_N ) );
      action->setWhatsThis( i18n( "FIXME Save all attachments." ) );
      d->mActions.insert( SaveAllAttachments, action );
      d->mActionCollection->addAction( QString::fromLatin1( "save_all_attachments" ), action );
      connect( action, SIGNAL( triggered( bool ) ), this, SLOT( slotSaveAllAttachments() ) );
      break;
    default:
      Q_ASSERT( false ); // should never happen
      break;
  }

  return action;
}

KAction* StandardCalendarActionManager::createAction( StandardActionManager::Type type )
{
  return d->mGenericManager->createAction( type );
}

void StandardCalendarActionManager::createAllActions()
{
  createAction( CreateEvent );

  d->mGenericManager->createAllActions();
  d->updateActions();
}

KAction* StandardCalendarActionManager::action( StandardCalendarActionManager::Type type ) const
{
  if ( d->mActions.contains( type ) )
    return d->mActions.value( type );

  return 0;
}

KAction* StandardCalendarActionManager::action( StandardActionManager::Type type ) const
{
  return d->mGenericManager->action( type );
}

void StandardCalendarActionManager::interceptAction( StandardCalendarActionManager::Type type, bool intercept )
{
  if ( intercept )
    d->mInterceptedActions.insert( type );
  else
    d->mInterceptedActions.remove( type );
}

void StandardCalendarActionManager::interceptAction( StandardActionManager::Type type, bool intercept )
{
  d->mGenericManager->interceptAction( type, intercept );
}

Akonadi::Collection::List StandardCalendarActionManager::selectedCollections() const
{
  return d->mGenericManager->selectedCollections();
}

Akonadi::Item::List StandardCalendarActionManager::selectedItems() const
{
  return d->mGenericManager->selectedItems();
}

#include "standardcalendaractionmanager.moc"
