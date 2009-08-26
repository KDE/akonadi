/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

#include "addressbookcombobox.h"

#include "collectionfiltermodel_p.h"

#include <akonadi/collectionfetchscope.h>
#include <akonadi/entitytreemodel.h>
#include <akonadi/entityfilterproxymodel.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>
#include <kabc/addressee.h>
#include <kabc/contactgroup.h>

#include <kdescendantsproxymodel.h>

#include <QtCore/QAbstractItemModel>
#include <QtGui/QComboBox>
#include <QtGui/QVBoxLayout>

using namespace Akonadi;

class AddressBookComboBox::Private
{
  public:
    Private( AddressBookComboBox::Type type, AddressBookComboBox *parent )
      : mParent( parent )
    {
      mComboBox = new QComboBox;

      QStringList contentTypes;
      switch ( type ) {
        case AddressBookComboBox::ContactsOnly: contentTypes << KABC::Addressee::mimeType(); break;
        case AddressBookComboBox::ContactGroupsOnly: contentTypes << KABC::ContactGroup::mimeType(); break;
        case AddressBookComboBox::All: contentTypes << KABC::Addressee::mimeType()
                                                    << KABC::ContactGroup::mimeType(); break;
      }

      mMonitor = new Akonadi::Monitor;
      mMonitor->fetchCollection( true );
      mMonitor->setCollectionMonitored( Akonadi::Collection::root() );

      mMonitor->collectionFetchScope().setContentMimeTypes( contentTypes );
      foreach ( const QString &mimeType, contentTypes )
        mMonitor->setMimeTypeMonitored( mimeType, true );

      mModel = new EntityTreeModel( Session::defaultSession(), mMonitor );

      EntityFilterProxyModel *filter = new EntityFilterProxyModel( parent );
      filter->setSourceModel( mModel );
      filter->addMimeTypeInclusionFilter( Akonadi::Collection::mimeType() );
      filter->setHeaderSet( EntityTreeModel::CollectionTreeHeaders );

      KDescendantsProxyModel *descProxy = new KDescendantsProxyModel( parent );
      descProxy->setSourceModel( filter );

      // filter for collections that support saving of contacts / contact groups
      CollectionFilterModel *filterModel = new CollectionFilterModel( mParent );
      foreach ( const QString &contentMimeType, contentTypes )
        filterModel->addContentMimeTypeFilter( contentMimeType );
      filterModel->setRightsFilter( Akonadi::Collection::CanCreateItem );
      filterModel->setSourceModel( descProxy );

      mComboBox->setModel( filterModel );
    }

    ~Private()
    {
      delete mModel;
      delete mMonitor;
    }

    void activated( int index );

    AddressBookComboBox *mParent;

    QComboBox *mComboBox;
    Monitor *mMonitor;
    EntityTreeModel *mModel;
};

void AddressBookComboBox::Private::activated( int index )
{
  const QModelIndex modelIndex = mComboBox->model()->index( index, 0 );
  if ( modelIndex.isValid() )
    emit mParent->selectionChanged( modelIndex.data( EntityTreeModel::CollectionRole).value<Collection>() );
}

AddressBookComboBox::AddressBookComboBox( Type type, QWidget *parent )
  : QWidget( parent ), d( new Private( type, this ) )
{
  QVBoxLayout *layout = new QVBoxLayout( this );
  layout->setMargin( 0 );
  layout->setSpacing( 0 );

  layout->addWidget( d->mComboBox );
  d->mComboBox->setCurrentIndex( 0 );

  connect( d->mComboBox, SIGNAL( activated( int ) ), SLOT( activated( int ) ) );
}

AddressBookComboBox::~AddressBookComboBox()
{
  delete d->mComboBox; // delete as long as the model still exists, crashs otherwise
  delete d;
}

Akonadi::Collection AddressBookComboBox::selectedAddressBook() const
{
  Q_ASSERT_X( d->mComboBox->model() != 0, "AddressBookComboBox::selectedAddressBook", "No model set!" );

  const int index = d->mComboBox->currentIndex();

  const QModelIndex modelIndex = d->mComboBox->model()->index( index, 0 );
  if ( modelIndex.isValid() )
    return modelIndex.data( Akonadi::EntityTreeModel::CollectionRole ).value<Collection>();
  else
    return Akonadi::Collection();
}

#include "addressbookcombobox.moc"
