/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "entitytreeviewstatesaver.h"

#include <akonadi/collection.h>
#include <akonadi/entitytreemodel.h>
#include <akonadi/item.h>

#include <KConfigGroup>
#include <KDebug>

#include <QHash>
#include <QPair>
#include <QScrollBar>
#include <QTimer>
#include <QTreeView>
#include <QVector>

namespace {
  struct AdditionalRole {
    QByteArray identifier;
    int role;
    QVariant defaultValue;
  };
}
namespace Akonadi {

struct State
{
  State() : selected( false ), expanded( false ), currentIndex( false ) {}
  bool selected;
  bool expanded;
  bool currentIndex;
  QHash<int, QVariant> additionalRoles;
};

class EntityTreeViewStateSaverPrivate
{
  public:
    explicit EntityTreeViewStateSaverPrivate( EntityTreeViewStateSaver *parent ) :
      q( parent ),
      view( 0 ),
      horizontalScrollBarValue( -1 ),
      verticalScrollBarValue( -1 )
    {
    }

    inline bool hasChanges() const
    {
      return !pendingCollectionChanges.isEmpty() || !pendingItemChanges.isEmpty();
    }

    static inline QString key( const QModelIndex &index )
    {
      if ( !index.isValid() )
        return QLatin1String( "x-1" );
      const Collection c = index.data( EntityTreeModel::CollectionRole ).value<Collection>();
      if ( c.isValid() )
        return QString::fromLatin1( "c%1" ).arg( c.id() );
      return QString::fromLatin1( "i%1" ).arg( index.data( EntityTreeModel::ItemIdRole ).value<Entity::Id>() );
    }

    void saveState( const QModelIndex &index, QStringList &selection, QStringList &expansion, QHash<QByteArray,QVector<QPair<QString, QVariant> > > &additionalValues )
    {
      const QString cfgKey = key( index );
      if ( view->selectionModel()->isSelected( index ) )
        selection.append( cfgKey );
      if ( view->isExpanded( index ) )
        expansion.append( cfgKey );
      Q_FOREACH( const AdditionalRole &ar, additionalRoles )
      {
        const QVariant v = index.data( ar.role );
        if ( v != ar.defaultValue )
          additionalValues[ar.identifier].push_back( qMakePair( cfgKey, v ) );
      }

      for ( int i = 0; i < view->model()->rowCount( index ); ++i ) {
        const QModelIndex child = view->model()->index( i, 0, index );
        saveState( child, selection, expansion, additionalValues );
      }
    }

    inline void restoreState( const QModelIndex &index, const State &state )
    {
      if ( state.selected )
        view->selectionModel()->select( index, QItemSelectionModel::Select | QItemSelectionModel::Rows );
      if ( state.expanded )
        view->setExpanded( index, true );
      if ( state.currentIndex )
        view->setCurrentIndex( index );
      QHash<int,QVariant>::ConstIterator it = state.additionalRoles.constBegin();
      while ( it != state.additionalRoles.constEnd() ) {
        if ( index.data( it.key() ) != it.value() )
          view->model()->setData( index, it.value(), it.key() );
        ++it;
      }
      QTimer::singleShot( 0, q, SLOT(restoreScrollBarState()) );
    }

    void restoreState( const QModelIndex &index )
    {
      const Collection c = index.data( EntityTreeModel::CollectionRole ).value<Collection>();
      if ( c.isValid() ) {
        if ( pendingCollectionChanges.contains( c.id() ) ) {
          restoreState( index, pendingCollectionChanges.value( c.id() ) );
          pendingCollectionChanges.remove( c.id() );
        }
      } else {
        Entity::Id itemId = index.data( EntityTreeModel::ItemIdRole ).value<Entity::Id>();
        if ( pendingItemChanges.contains( itemId ) ) {
          restoreState( index, pendingItemChanges.value( itemId ) );
          pendingItemChanges.remove( itemId );
        }
      }
      for ( int i = 0; i < view->model()->rowCount( index ) && hasChanges(); ++i ) {
        const QModelIndex child = view->model()->index( i, 0, index );
        restoreState( child );
      }
    }

    inline void restoreScrollBarState()
    {
      if ( horizontalScrollBarValue >= 0 && horizontalScrollBarValue <= view->horizontalScrollBar()->maximum() ) {
        view->horizontalScrollBar()->setValue( horizontalScrollBarValue );
        horizontalScrollBarValue = -1;
      }
      if ( verticalScrollBarValue >= 0 && verticalScrollBarValue <= view->verticalScrollBar()->maximum() ) {
        view->verticalScrollBar()->setValue( verticalScrollBarValue );
        verticalScrollBarValue = -1;
      }
    }

    void rowsInserted( const QModelIndex &index, int start, int end )
    {
      if ( !hasChanges() ) {
        QObject::disconnect( view->model(), SIGNAL(rowsInserted(QModelIndex,int,int)), q, SLOT(rowsInserted(QModelIndex,int,int)) );
        return;
      }

      for ( int i = start; i <= end && hasChanges(); ++i ) {
        const QModelIndex child = view->model()->index( i, 0, index);;
        restoreState( child );
      }
    }

    EntityTreeViewStateSaver *q;
    QTreeView *view;
    QHash<Entity::Id, State> pendingCollectionChanges, pendingItemChanges;
    int horizontalScrollBarValue, verticalScrollBarValue;
    QHash<QByteArray,AdditionalRole> additionalRoles;
};

EntityTreeViewStateSaver::EntityTreeViewStateSaver( QTreeView * view ) :
  QObject( view ),
  d( new EntityTreeViewStateSaverPrivate( this ) )
{
  d->view = view;
}

EntityTreeViewStateSaver::~EntityTreeViewStateSaver()
{
  delete d;
}

void EntityTreeViewStateSaver::addAdditionalRole( int role, const QByteArray &identifier, const QVariant &defaultValue )
{
  AdditionalRole ar;
  ar.role = role;
  ar.identifier = identifier;
  ar.defaultValue = defaultValue;
  d->additionalRoles.insert( identifier, ar );
}

void EntityTreeViewStateSaver::saveState( KConfigGroup &configGroup ) const
{
  configGroup.deleteGroup();
  QStringList selection, expansion;
  typedef QPair<QString, QVariant> StringVariantPair;
  typedef QHash<QByteArray, QVector<StringVariantPair> > ValueHash;
  ValueHash additionalValues;

  for ( int i = 0; i < d->view->model()->rowCount(); ++i ) {
    const QModelIndex index = d->view->model()->index( i, 0 );
    d->saveState( index, selection, expansion, additionalValues );
  }

  const QString currentIndex = d->key( d->view->selectionModel()->currentIndex() );

  configGroup.writeEntry( "Selection", selection );
  configGroup.writeEntry( "Expansion", expansion );
  configGroup.writeEntry( "CurrentIndex", currentIndex );
  configGroup.writeEntry( "ScrollBarHorizontal", d->view->horizontalScrollBar()->value() );
  configGroup.writeEntry( "ScrollBarVertical", d->view->verticalScrollBar()->value() );
  ValueHash::ConstIterator it = additionalValues.constBegin();
  while ( it != additionalValues.constEnd() ) {
    QStringList l;
    Q_FOREACH( const StringVariantPair &pair, it.value() ) {
      l.push_back( pair.first );
      l.push_back( pair.second.toString() );
    }
    configGroup.writeEntry( ( QByteArray("AdditionalRole_") + it.key() ).constData(), l );
    ++it;
  }
}

void EntityTreeViewStateSaver::restoreState (const KConfigGroup & configGroup) const
{
  const QStringList selection = configGroup.readEntry( "Selection", QStringList() );
  foreach( const QString &key, selection ) {
    Entity::Id id = key.mid( 1 ).toLongLong();
    if ( id < 0 )
      continue;
    if ( key.startsWith( QLatin1Char( 'c' ) ) )
      d->pendingCollectionChanges[id].selected = true;
    else if ( key.startsWith( QLatin1Char( 'i' ) ) )
      d->pendingItemChanges[id].selected = true;
  }

  const QStringList expansion = configGroup.readEntry( "Expansion", QStringList() );
  foreach( const QString &key, expansion ) {
    Entity::Id id = key.mid( 1 ).toLongLong();
    if ( id < 0 )
      continue;
    if ( key.startsWith( QLatin1Char( 'c' ) ) )
      d->pendingCollectionChanges[id].expanded = true;
    else if ( key.startsWith( QLatin1Char( 'i' ) ) )
      d->pendingItemChanges[id].expanded = true;
  }

  const QString key = configGroup.readEntry( "CurrentIndex", QString() );
  const Entity::Id id = key.mid( 1 ).toLongLong();
  if ( id >= 0 ) {
    if ( key.startsWith( QLatin1Char( 'c' ) ) )
      d->pendingCollectionChanges[id].currentIndex = true;
    else if ( key.startsWith( QLatin1Char( 'i' ) ) )
      d->pendingItemChanges[id].currentIndex = true;
  }

  d->horizontalScrollBarValue = configGroup.readEntry( "ScrollBarHorizontal", -1 );
  d->verticalScrollBarValue = configGroup.readEntry( "ScrollBarVertical", -1 );

  Q_FOREACH ( const AdditionalRole &ar, d->additionalRoles ) {
    const QStringList l = configGroup.readEntry( ( QByteArray("AdditionalRole_") + ar.identifier ).constData(), QStringList() );
    if ( l.isEmpty() || l.size() % 2 != 0 ) //lists with an odd number of entries are invalid and ignored
      continue;
    QStringList::ConstIterator it = l.constBegin();
    while ( it != l.constEnd() ) {
      const QString key = *it;
      ++it;
      const QVariant value = *it;
      ++it;
      if ( value == ar.defaultValue )
        continue;
      const Entity::Id id = key.mid( 1 ).toLongLong();
      if ( id >= 0 ) {
        if ( key.startsWith( QLatin1Char( 'c' ) ) )
          d->pendingCollectionChanges[id].additionalRoles.insert( ar.role, value );
        else if ( key.startsWith( QLatin1Char( 'i' ) ) )
          d->pendingItemChanges[id].additionalRoles.insert( ar.role, value );
      }
    }
  }

  // initial restore run, for everything already loaded
  for ( int i = 0; i < d->view->model()->rowCount() && d->hasChanges(); ++i ) {
    const QModelIndex index = d->view->model()->index( i, 0 );
    d->restoreState( index );
  }
  d->restoreScrollBarState();

  // watch the model for stuff coming in delayed
  if ( d->hasChanges() )
    connect( d->view->model(), SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(rowsInserted(QModelIndex,int,int)), Qt::QueuedConnection );
}

} // namespace Akonadi

#include "entitytreeviewstatesaver.moc"
