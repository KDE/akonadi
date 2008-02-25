/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "standardactionmanager.h"

#include <libakonadi/agentmanager.h>
#include <libakonadi/collectioncreatejob.h>
#include <libakonadi/collectiondeletejob.h>
#include <libakonadi/collectionmodel.h>
#include <libakonadi/collectionpropertiesdialog.h>

#include <KAction>
#include <KActionCollection>
#include <KDebug>
#include <KInputDialog>
#include <KLocale>
#include <KMessageBox>

#include <QApplication>
#include <QClipboard>
#include <QItemSelectionModel>
#include <QMimeData>

#include <boost/static_assert.hpp>

using namespace Akonadi;

static const struct {
  const char *name;
  const char *label;
  const char *icon;
  int shortcut;
  const char* slot;
} actionData[] = {
  { "akonadi_collection_create", I18N_NOOP("&New Folder..."), "folder-new", 0, SLOT(slotCreateCollection()) },
  { "akonadi_collection_copy", I18N_NOOP("&Copy Folder"), "edit-copy", 0, SLOT(slotCopyCollection()) },
  { "akonadi_collection_delete", I18N_NOOP("&Delete Folder"), "edit-delete", 0, SLOT(slotDeleteCollection()) },
  { "akonadi_collection_sync", I18N_NOOP("&Synchronize Folder"), "view-refresh", Qt::Key_F5, SLOT(slotSynchronizeCollection()) },
  { "akonadi_collection_properties", I18N_NOOP("Folder &Properties..."), "configure", 0, SLOT(slotCollectionProperties()) }
};
static const int numActionData = sizeof actionData / sizeof *actionData;

BOOST_STATIC_ASSERT( numActionData == StandardActionManager::LastType );

class StandardActionManager::Private
{
  public:
    Private( StandardActionManager *parent ) :
      q( parent ),
      collectionSelectionModel( 0 )
    {
      actions.fill( 0, StandardActionManager::LastType );
      agentManager = new AgentManager( parent );
    }

    void enableAction( StandardActionManager::Type type, bool enable )
    {
      Q_ASSERT( type >= 0 && type < StandardActionManager::LastType );
      if ( actions[type] )
        actions[type]->setEnabled( enable );
    }

    void collectionSelectionChanged( const QItemSelection &selected, const QItemSelection &deselected )
    {
      Q_UNUSED( selected );
      Q_UNUSED( deselected );
      updateActions();
    }

    void updateActions()
    {
      bool singleColSelected = false;
      QModelIndex selectedIndex;
      if ( collectionSelectionModel ) {
        singleColSelected = collectionSelectionModel->selectedRows().count() == 1;
        if ( singleColSelected )
          selectedIndex = collectionSelectionModel->selectedRows().first();
      }

      enableAction( CopyCollection, singleColSelected );
      enableAction( CollectionProperties, singleColSelected );

      if ( singleColSelected && selectedIndex.isValid() ) {
        const Collection col = selectedIndex.data( CollectionModel::CollectionRole ).value<Collection>();
        enableAction( CreateCollection, selectedIndex.data( CollectionModel::ChildCreatableRole ).toBool() );
        enableAction( DeleteCollection, col.type() == Collection::Folder );
        enableAction( SynchronizeCollection, col.type() == Collection::Folder || col.type() == Collection::Resource );
      } else {
        enableAction( CreateCollection, false );
        enableAction( DeleteCollection, false );
        enableAction( SynchronizeCollection, false );
      }
    }

    void slotCreateCollection()
    {
      const QModelIndex index = collectionSelectionModel->currentIndex();
      if ( !index.data( CollectionModel::ChildCreatableRole ).toBool() )
        return;
      const QString name = KInputDialog::getText( i18nc( "@title:window", "New Folder"),
                                                  i18nc( "@label:textbox, name of a thing", "Name"),
                                                  QString(), 0, parentWidget );
      if ( name.isEmpty() )
        return;
      int parentId = index.data( CollectionModel::CollectionIdRole ).toInt();
      if ( parentId <= 0 )
        return;

      Collection col;
      col.setName( name );
      col.setParent( parentId );
      CollectionCreateJob *job = new CollectionCreateJob( col );
      q->connect( job, SIGNAL(result(KJob*)), q, SLOT(collectionCreationResult(KJob*)) );
    }

    void slotCopyCollection()
    {
      Q_ASSERT( collectionSelectionModel );
      if ( collectionSelectionModel->selectedRows().count() <= 0 )
        return;
      QMimeData *mimeData = collectionSelectionModel->model()->mimeData( collectionSelectionModel->selectedRows() );
      QApplication::clipboard()->setMimeData( mimeData );
    }

    void slotDeleteCollection()
    {
      Q_ASSERT( collectionSelectionModel );
      const QModelIndex index = collectionSelectionModel->currentIndex();
      if ( !index.isValid() )
        return;
      if ( KMessageBox::questionYesNo( parentWidget,
           i18n( "Do you really want to delete folder '%1' and all its sub-folders?", index.data().toString() ),
           i18n("Delete folder?"), KStandardGuiItem::del(), KStandardGuiItem::cancel(),
           QString(), KMessageBox::Dangerous ) != KMessageBox::Yes )
        return;
      const int colId = index.data( CollectionModel::CollectionIdRole ).toInt();
      if ( colId <= 0 )
        return;

      CollectionDeleteJob *job = new CollectionDeleteJob( Collection( colId ), q );
      q->connect( job, SIGNAL(result(KJob*)), q, SLOT(collectionDeletionResult(KJob*)) );
    }

    void slotSynchronizeCollection()
    {
      QModelIndex index = collectionSelectionModel->currentIndex();
      if ( !index.isValid() )
        return;
      const Collection col = index.data( CollectionModel::CollectionRole ).value<Collection>();
      agentManager->agentInstanceSynchronizeCollection( col );
    }

    void slotCollectionProperties()
    {
      const QModelIndex index = collectionSelectionModel->currentIndex();
      if ( !index.isValid() )
        return;
      const Collection col = index.data( CollectionModel::CollectionRole ).value<Collection>();
      CollectionPropertiesDialog* dlg = new CollectionPropertiesDialog( col, parentWidget );
      dlg->show();
    }

    void collectionCreationResult( KJob *job )
    {
      if ( job->error() ) {
        KMessageBox::error( parentWidget, i18n("Could not create folder: %1", job->errorString()),
                            i18n("Folder creation failed") );
      }
    }

    void collectionDeletionResult( KJob *job )
    {
      if ( job->error() ) {
        KMessageBox::error( parentWidget, i18n("Could not delete folder: %1", job->errorString()),
                            i18n("Folder deletion failed") );
      }
    }

    StandardActionManager *q;
    KActionCollection *actionCollection;
    QWidget *parentWidget;
    QItemSelectionModel *collectionSelectionModel;
    QVector<KAction*> actions;
    AgentManager *agentManager;
};

StandardActionManager::StandardActionManager( KActionCollection * actionCollection,
                                              QWidget * parent) :
    QObject( parent ),
    d( new Private( this ) )
{
  d->parentWidget = parent;
  d->actionCollection = actionCollection;
}

StandardActionManager::~ StandardActionManager()
{
  delete d;
}

void StandardActionManager::setCollectionSelectionModel(QItemSelectionModel * selectionModel)
{
  d->collectionSelectionModel = selectionModel;
  connect( selectionModel, SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
           SLOT(collectionSelectionChanged(QItemSelection, QItemSelection)) );
}

KAction* StandardActionManager::createAction( Type type )
{
  Q_ASSERT( type >= 0 && type < LastType );
  Q_ASSERT( actionData[type].name );
  if ( d->actions[type] )
    return d->actions[type];
  KAction *action = new KAction( d->parentWidget );
  if ( actionData[type].label )
    action->setText( i18n( actionData[type].label ) );
  if ( actionData[type].icon )
    action->setIcon( KIcon( QString::fromLatin1( actionData[type].icon ) ) );
  action->setShortcut( actionData[type].shortcut );
  if ( actionData[type].slot )
    connect( action, SIGNAL(triggered()), actionData[type].slot );
  d->actionCollection->addAction( QString::fromLatin1(actionData[type].name), action );
  d->actions[type] = action;
  d->updateActions();
  return action;
}

void StandardActionManager::createAllActions()
{
  for ( int i = 0; i < LastType; ++i )
    createAction( (Type)i );
}

KAction * StandardActionManager::action( Type type ) const
{
  Q_ASSERT( type >= 0 && type < LastType );
  return d->actions[type];
}

#include "standardactionmanager.moc"
