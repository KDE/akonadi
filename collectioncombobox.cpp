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

      mEntityMimeTypeFilterModel = new EntityMimeTypeFilterModel( parent );
      mEntityMimeTypeFilterModel->setSourceModel( baseModel );

/*
      QStringList contentTypes;
      switch ( type ) {
        case CollectionComboBox::ContactsOnly: contentTypes << KABC::Addressee::mimeType(); break;
        case CollectionComboBox::ContactGroupsOnly: contentTypes << KABC::ContactGroup::mimeType(); break;
        case CollectionComboBox::All: contentTypes << KABC::Addressee::mimeType()
                                                    << KABC::ContactGroup::mimeType(); break;
      }
      // filter for collections that support saving of contacts / contact groups
      CollectionFilterModel *filterModel = new CollectionFilterModel( mParent );
      foreach ( const QString &contentMimeType, contentTypes )
        filterModel->addContentMimeTypeFilter( contentMimeType );

      if ( rights == CollectionComboBox::Writable )
        filterModel->setRightsFilter( Akonadi::Collection::CanCreateItem );

      filterModel->setSourceModel( proxyModel );
*/

      mParent->setModel( mEntityMimeTypeFilterModel );

      mSelectionHandler = new AsyncSelectionHandler( mEntityMimeTypeFilterModel, mParent );
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
    EntityMimeTypeFilterModel *mEntityMimeTypeFilterModel;
    AsyncSelectionHandler *mSelectionHandler;

    QStringList mMimeTypesFilter;
    CollectionComboBox::AccessRights mAccessRightsFilter; 
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

void CollectionComboBox::setContentMimeTypesFilter( const QStringList &contentMimeTypes )
{
  d->mEntityMimeTypeFilterModel->clearFilters();
  d->mEntityMimeTypeFilterModel->addContentMimeTypeInclusionFilters( contentMimeTypes );
  d->mMimeTypesFilter = contentMimeTypes;

  if ( d->mMonitor ) {
    d->mMonitor->collectionFetchScope().setContentMimeTypes( contentMimeTypes );
    foreach ( const QString &mimeType, contentMimeTypes )
      d->mMonitor->setMimeTypeMonitored( mimeType, true );
  }
}

QStringList CollectionComboBox::contentMimeTypesFilter() const
{
  return d->mMimeTypesFilter;
}

void CollectionComboBox::setAccessRightsFilter( AccessRights rights )
{
  d->mAccessRightsFilter = rights;
}

CollectionComboBox::AccessRights CollectionComboBox::accessRightsFilter() const
{
  return d->mAccessRightsFilter;
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
