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

#include <QtCore/QAbstractItemModel>
#include <QtGui/QComboBox>
#include <QtGui/QVBoxLayout>

#include "collection.h"
#include "collectionmodel.h"

#include "collectioncombobox.h"

using namespace Akonadi;

class CollectionComboBox::Private
{
  public:
    Private( CollectionComboBox *parent )
      : mParent( parent )
    {
    }

    void activated( int index );

    CollectionComboBox *mParent;

    QComboBox *mComboBox;
};

void CollectionComboBox::Private::activated( int index )
{
  if ( !mComboBox->model() )
    return;

  const QModelIndex modelIndex = mComboBox->model()->index( index, 0 );
  if ( modelIndex.isValid() )
    emit mParent->selectionChanged( Collection( modelIndex.data( CollectionModel::CollectionIdRole ).toInt() ) );
}

CollectionComboBox::CollectionComboBox( QWidget *parent )
  : QWidget( parent ), d( new Private( this ) )
{
  QVBoxLayout *layout = new QVBoxLayout( this );
  layout->setMargin( 0 );
  layout->setSpacing( 0 );

  d->mComboBox = new QComboBox( this );
  layout->addWidget( d->mComboBox );

  connect( d->mComboBox, SIGNAL( activated( int ) ), SLOT( activated( int ) ) );
}

CollectionComboBox::~CollectionComboBox()
{
  delete d;
}

void CollectionComboBox::setModel( QAbstractItemModel *model )
{
  d->mComboBox->setModel( model );
}

Collection CollectionComboBox::selectedCollection() const
{
  Q_ASSERT_X( d->mComboBox->model() != 0, "CollectionComboBox::selectionChanged", "No model set!" );

  int index = d->mComboBox->currentIndex();

  const QModelIndex modelIndex = d->mComboBox->model()->index( index, 0 );
  if ( modelIndex.isValid() )
    return Collection( modelIndex.data( CollectionModel::CollectionIdRole ).toInt() );
  else
    return Collection();
}

#include "collectioncombobox.moc"
