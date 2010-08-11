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

      //TODO set generic action strings
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
  : QObject( actionCollection, parent ),
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
  Q_ASSERT( type >= CreateEvent && type < LastType );
  if ( d->mActions.contains( type ) )
    return d->mActions.value( type );

  KAction *action = 0;
  switch ( type ) {
    case CreateEvent:
      action = new KAction( d->mParentWidget );
      action->setIcon( KIcon( QLatin1String( "event-new" ) ) );
      action->setText( i18n( "New &Event..." ) );
      action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_N ) );
      action->setWhatsThis( i18n( "Create a new event" ) );
      d->mActions.insert( CreateEvent, action );
      d->mActionCollection->addAction( QString::fromLatin1( "akonadi_event_create" ), action );
      connect( action, SIGNAL( triggered( bool ) ), this, SLOT( slotCreateEvent() ) );
      break;
    default:
      Q_ASSERT( false ); // should never happen
      break;
  }
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
  else
    return 0;
}

void StandardCalendarActionManager::interceptAction( StandardCalendarActionManager::Type type, bool intercept )
{
  if ( type <= StandardActionManager::LastType ) {
      d->mGenericManager->interceptAction( static_cast<StandardActionManager::Type>( type ), intercept );
      return;
  }

  if ( type >= StandardCalendarActionManager::CreateEvent && type <= StandardCalendarActionManager::LastType ) {
    if ( intercept )
      d->mInterceptedActions.insert( type );
    else
      d->mInterceptedActions.remove( type );
  }
}

Akonadi::Collection::List StandardCalendarActionManager::selectedCollections() const
{
  return d->mGenericManager->selectedCollections();
}

Akonadi::Item::List StandardCalendarActionManager::selectedItems() const
{
  return d->mGenericManager->selectedItems();
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
