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
#include <libakonadi/itemdeletejob.h>
#include <libakonadi/itemmodel.h>
#include <libakonadi/pastehelper.h>
#include <libakonadi/subscriptiondialog.h>

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
  { "akonadi_collection_copy", 0, "edit-copy", 0, SLOT(slotCopyCollections()) },
  { "akonadi_collection_delete", I18N_NOOP("&Delete Folder"), "edit-delete", 0, SLOT(slotDeleteCollection()) },
  { "akonadi_collection_sync", I18N_NOOP("&Synchronize Folder"), "view-refresh", Qt::Key_F5, SLOT(slotSynchronizeCollection()) },
  { "akonadi_collection_properties", I18N_NOOP("Folder &Properties..."), "configure", 0, SLOT(slotCollectionProperties()) },
  { "akonadi_item_copy", 0, "edit-copy", 0, SLOT(slotCopyItems()) },
  { "akonadi_paste", I18N_NOOP("&Paste"), "edit-paste", Qt::CTRL + Qt::Key_V, SLOT(slotPaste()) },
  { "akonadi_item_delete", 0, "edit-delete", Qt::Key_Delete, SLOT(slotDeleteItems()) },
  { "akonadi_manage_local_subscriptions", I18N_NOOP("Manage Local &Subscriptions..."), 0, 0, SLOT(slotLocalSubscription()) }
};
static const int numActionData = sizeof actionData / sizeof *actionData;

BOOST_STATIC_ASSERT( numActionData == StandardActionManager::LastType );

class StandardActionManager::Private
{
  public:
    Private( StandardActionManager *parent ) :
      q( parent ),
      collectionSelectionModel( 0 ),
      itemSelectionModel( 0 )
    {
      actions.fill( 0, StandardActionManager::LastType );
      agentManager = new AgentManager( parent );

      pluralLabels.insert( StandardActionManager::CopyCollections, ki18np( "&Copy Folder", "&Copy %1 Folders" ) );
      pluralLabels.insert( StandardActionManager::CopyItems, ki18np( "&Copy Item", "&Copy %1 Items" ) );
      pluralLabels.insert( StandardActionManager::DeleteItems, ki18np( "&Delete Item", "&Delete %1 Items" ) );
    }

    void enableAction( StandardActionManager::Type type, bool enable )
    {
      Q_ASSERT( type >= 0 && type < StandardActionManager::LastType );
      if ( actions[type] )
        actions[type]->setEnabled( enable );
    }

    void updatePluralLabel( StandardActionManager::Type type, int count )
    {
      Q_ASSERT( type >= 0 && type < StandardActionManager::LastType );
      if ( actions[type] && pluralLabels.contains( type ) && !pluralLabels.value( type ).isEmpty() ) {
        actions[type]->setText( pluralLabels.value( type ).subs( qMax( count, 1 ) ).toString() );
      }
    }

    void copy( QItemSelectionModel* selModel )
    {
      Q_ASSERT( selModel );
      if ( selModel->selectedRows().count() <= 0 )
        return;
      QMimeData *mimeData = selModel->model()->mimeData( selModel->selectedRows() );
      QApplication::clipboard()->setMimeData( mimeData );
    }

    void updateActions()
    {
      bool singleColSelected = false;
      bool multiColSelected = false;
      int colCount = 0;
      QModelIndex selectedIndex;
      if ( collectionSelectionModel ) {
        colCount = collectionSelectionModel->selectedRows().count();
        singleColSelected = colCount == 1;
        multiColSelected = colCount > 0;
        if ( singleColSelected )
          selectedIndex = collectionSelectionModel->selectedRows().first();
      }

      enableAction( CopyCollections, multiColSelected );
      enableAction( CollectionProperties, singleColSelected );

      if ( singleColSelected && selectedIndex.isValid() ) {
        const Collection col = selectedIndex.data( CollectionModel::CollectionRole ).value<Collection>();
        enableAction( CreateCollection, selectedIndex.data( CollectionModel::ChildCreatableRole ).toBool() );
        enableAction( DeleteCollections, col.type() == Collection::Folder || col.type() == Collection::Structural );
        enableAction( SynchronizeCollections, col.type() == Collection::Folder || col.type() == Collection::Resource );
        enableAction( Paste, PasteHelper::canPaste( QApplication::clipboard()->mimeData(), col ) );
      } else {
        enableAction( CreateCollection, false );
        enableAction( DeleteCollections, false );
        enableAction( SynchronizeCollections, false );
        enableAction( Paste, false );
      }

      bool multiItemSelected = false;
      int itemCount = 0;
      if ( itemSelectionModel ) {
        itemCount = itemSelectionModel->selectedRows().count();
        multiItemSelected = itemCount > 0;
      }

      enableAction( CopyItems, multiItemSelected );
      enableAction( DeleteItems, multiItemSelected );

      updatePluralLabel( CopyCollections, colCount );
      updatePluralLabel( CopyItems, itemCount );
      updatePluralLabel( DeleteItems, itemCount );

      emit q->actionStateUpdated();
    }

    void clipboardChanged( QClipboard::Mode mode )
    {
      if ( mode == QClipboard::Clipboard )
        updateActions();
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

    void slotCopyCollections()
    {
      copy( collectionSelectionModel );
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

    void slotCopyItems()
    {
      copy( itemSelectionModel );
    }

    void slotPaste()
    {
      const QModelIndex index = collectionSelectionModel->currentIndex();
      if ( !index.isValid() )
        return;
      const Collection col = index.data( CollectionModel::CollectionRole ).value<Collection>();
      QList<KJob*> jobs = PasteHelper::paste( QApplication::clipboard()->mimeData(), col );
      foreach ( KJob* job, jobs )
        q->connect( job, SIGNAL(result(KJob*)), q, SLOT(pasteResult(KJob*)) );
    }

    void slotDeleteItems()
    {
      if ( KMessageBox::questionYesNo( parentWidget,
           i18n( "Do you really want to delete all selected items?" ),
           i18n("Delete?"), KStandardGuiItem::del(), KStandardGuiItem::cancel(),
           QString(), KMessageBox::Dangerous ) != KMessageBox::Yes )
        return;

      Q_ASSERT( itemSelectionModel );

      // TODO: fix this once ItemStoreJob can handle item lists
      foreach ( const QModelIndex index, itemSelectionModel->selectedRows() ) {
        new ItemDeleteJob( DataReference( index.data( ItemModel::IdRole ).toInt(), QString() ), q );
      }
    }

    void slotLocalSubscription()
    {
      SubscriptionDialog* dlg = new SubscriptionDialog( parentWidget );
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

    void pasteResult( KJob *job )
    {
      if ( job->error() ) {
        KMessageBox::error( parentWidget, i18n("Could not paste data: %1", job->errorString()),
                            i18n("Paste failed") );
      }
    }

    StandardActionManager *q;
    KActionCollection *actionCollection;
    QWidget *parentWidget;
    QItemSelectionModel *collectionSelectionModel;
    QItemSelectionModel *itemSelectionModel;
    QVector<KAction*> actions;
    AgentManager *agentManager;
    QHash<StandardActionManager::Type, KLocalizedString> pluralLabels;
};

StandardActionManager::StandardActionManager( KActionCollection * actionCollection,
                                              QWidget * parent) :
    QObject( parent ),
    d( new Private( this ) )
{
  d->parentWidget = parent;
  d->actionCollection = actionCollection;
  connect( QApplication::clipboard(), SIGNAL(changed(QClipboard::Mode)), SLOT(clipboardChanged(QClipboard::Mode)) );
}

StandardActionManager::~ StandardActionManager()
{
  delete d;
}

void StandardActionManager::setCollectionSelectionModel(QItemSelectionModel * selectionModel)
{
  d->collectionSelectionModel = selectionModel;
  connect( selectionModel, SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
           SLOT(updateActions()) );
}

void StandardActionManager::setItemSelectionModel(QItemSelectionModel * selectionModel)
{
  d->itemSelectionModel = selectionModel;
  connect( selectionModel, SIGNAL(selectionChanged( const QItemSelection&, const QItemSelection& )),
           SLOT(updateActions()) );
}

KAction* StandardActionManager::createAction( Type type )
{
  Q_ASSERT( type >= 0 && type < LastType );
  Q_ASSERT( actionData[type].name );
  if ( d->actions[type] )
    return d->actions[type];
  KAction *action = new KAction( d->parentWidget );
  if ( d->pluralLabels.contains( type ) && !d->pluralLabels.value( type ).isEmpty() )
    action->setText( d->pluralLabels.value( type ).subs( 1 ).toString() );
  else if ( actionData[type].label )
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

void StandardActionManager::setActionText(Type type, const KLocalizedString & text)
{
  Q_ASSERT( type >= 0 && type < LastType );
  d->pluralLabels.insert( type, text );
}

#include "standardactionmanager.moc"
