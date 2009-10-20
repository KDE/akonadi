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

#include "collectioncombobox.h"

#include "asyncselectionhandler_p.h"

#include <akonadi/changerecorder.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/entitymimetypefiltermodel.h>
#include <akonadi/entityrightsfiltermodel.h>
#include <akonadi/entitytreemodel.h>
#include <akonadi/session.h>

#include <kdescendantsproxymodel.h>

#include <QtCore/QAbstractItemModel>

using namespace Akonadi;

class CollectionComboBox::Private
{
  public:
    Private( QAbstractItemModel *customModel, CollectionComboBox *parent )
      : mParent( parent ), mMonitor( 0 ), mModel( 0 )
    {
      QAbstractItemModel *baseModel;

      if ( customModel ) {
        baseModel = customModel;
      } else {
        mMonitor = new Akonadi::ChangeRecorder;
        mMonitor->fetchCollection( true );
        mMonitor->setCollectionMonitored( Akonadi::Collection::root() );

        mModel = new EntityTreeModel( Session::defaultSession(), mMonitor );
        mModel->setItemPopulationStrategy( EntityTreeModel::NoItemPopulation );

        KDescendantsProxyModel *proxyModel = new KDescendantsProxyModel( parent );
        proxyModel->setSourceModel( mModel );

        baseModel = proxyModel;
      }

      mMimeTypeFilterModel = new EntityMimeTypeFilterModel( parent );
      mMimeTypeFilterModel->setSourceModel( baseModel );

      mRightsFilterModel = new EntityRightsFilterModel( parent );
      mRightsFilterModel->setSourceModel( mMimeTypeFilterModel );

      mParent->setModel( mRightsFilterModel );

      mSelectionHandler = new AsyncSelectionHandler( mRightsFilterModel, mParent );
      mParent->connect( mSelectionHandler, SIGNAL( collectionAvailable( const QModelIndex& ) ),
                        mParent, SLOT( activated( const QModelIndex& ) ) );

      mParent->connect( mParent, SIGNAL( activated( int ) ),
                        mParent, SLOT( activated( int ) ) );
    }

    ~Private()
    {
      mParent->setModel( 0 );
      delete mModel;
      delete mMonitor;
    }

    void activated( int index );
    void activated( const QModelIndex& index );

    CollectionComboBox *mParent;

    ChangeRecorder *mMonitor;
    EntityTreeModel *mModel;
    EntityMimeTypeFilterModel *mMimeTypeFilterModel;
    EntityRightsFilterModel *mRightsFilterModel;
    AsyncSelectionHandler *mSelectionHandler;
};

void CollectionComboBox::Private::activated( int index )
{
  const QModelIndex modelIndex = mParent->model()->index( index, 0 );
  if ( modelIndex.isValid() )
    emit mParent->currentChanged( modelIndex.data( EntityTreeModel::CollectionRole).value<Collection>() );
}

void CollectionComboBox::Private::activated( const QModelIndex &index )
{
  mParent->setCurrentIndex( index.row() );
}


CollectionComboBox::CollectionComboBox( QWidget *parent )
  : KComboBox( parent ), d( new Private( 0, this ) )
{
}

CollectionComboBox::CollectionComboBox( QAbstractItemModel *model, QWidget *parent )
  : KComboBox( parent ), d( new Private( model, this ) )
{
}

CollectionComboBox::~CollectionComboBox()
{
  delete d;
}

void CollectionComboBox::setMimeTypeFilter( const QStringList &contentMimeTypes )
{
  d->mMimeTypeFilterModel->clearFilters();
  d->mMimeTypeFilterModel->addContentMimeTypeInclusionFilters( contentMimeTypes );

  if ( d->mMonitor ) {
    d->mMonitor->collectionFetchScope().setContentMimeTypes( contentMimeTypes );
    foreach ( const QString &mimeType, contentMimeTypes )
      d->mMonitor->setMimeTypeMonitored( mimeType, true );
  }
}

QStringList CollectionComboBox::mimeTypeFilter() const
{
  return d->mMimeTypeFilterModel->contentMimeTypeInclusionFilters();
}

void CollectionComboBox::setAccessRightsFilter( Collection::Rights rights )
{
  d->mRightsFilterModel->setAccessRights( rights );
}

Collection::Rights CollectionComboBox::accessRightsFilter() const
{
  return d->mRightsFilterModel->accessRights();
}

void CollectionComboBox::setDefaultCollection( const Collection &collection )
{
  d->mSelectionHandler->waitForCollection( collection );
}

Akonadi::Collection CollectionComboBox::currentCollection() const
{
  const QModelIndex modelIndex = model()->index( currentIndex(), 0 );
  if ( modelIndex.isValid() )
    return modelIndex.data( Akonadi::EntityTreeModel::CollectionRole ).value<Collection>();
  else
    return Akonadi::Collection();
}

#include "collectioncombobox.moc"
