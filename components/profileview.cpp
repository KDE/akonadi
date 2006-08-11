/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

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

#include <QtGui/QApplication>
#include <QtGui/QListView>
#include <QtGui/QVBoxLayout>

#include <libakonadi/profilemodel.h>

#include "profileview.h"

using namespace PIM;

class ProfileView::Private
{
  public:
    QListView *mView;
    ProfileModel *mModel;
};

ProfileView::ProfileView( QWidget *parent )
  : QWidget( parent ), d( new Private )
{
  QVBoxLayout *layout = new QVBoxLayout( this );
  layout->setMargin( 0 );
  layout->setSpacing( 0 );

  d->mView = new QListView( this );
  layout->addWidget( d->mView );

  d->mModel = new ProfileModel( d->mView );
  d->mView->setModel( d->mModel );

  d->mView->selectionModel()->select( d->mModel->index( 0, 0 ), QItemSelectionModel::Select );
}

ProfileView::~ProfileView()
{
  delete d;
}

QString ProfileView::currentProfile() const
{
  QItemSelectionModel *selectionModel = d->mView->selectionModel();
  if ( !selectionModel )
    return QString();

  QModelIndex index = selectionModel->currentIndex();
  if ( !index.isValid() )
    return QString();

  return index.model()->data( index, Qt::DisplayRole ).toString();
}
