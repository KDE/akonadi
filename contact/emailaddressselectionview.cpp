/*
    This file is part of Akonadi Contact.

    Copyright (c) 2010 KDAB
    Author: Tobias Koenig <tokoe@kde.org>

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

#include "emailaddressselectionview.h"

#include "leafextensionproxymodel.h"

#include <akonadi/changerecorder.h>
#include <akonadi/contact/contactsfiltermodel.h>
#include <akonadi/contact/contactstreemodel.h>
#include <akonadi/control.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/entitytreeview.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/session.h>
#include <kabc/addressee.h>
#include <kabc/contactgroup.h>
#include <klineedit.h>
#include <klocale.h>

#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>

using namespace Akonadi;

/**
 * @internal
 */
class EmailAddressSelectionView::Selection::Private : public QSharedData
{
  public:
    Private()
      : QSharedData()
    {
    }

    Private( const Private &other )
      : QSharedData( other )
    {
      mName = other.mName;
      mEmailAddress = other.mEmailAddress;
      mItem = other.mItem;
    }

    QString mName;
    QString mEmailAddress;
    Akonadi::Item mItem;
};

EmailAddressSelectionView::Selection::Selection()
  : d( new Private )
{
}

EmailAddressSelectionView::Selection::Selection( const Selection &other )
  : d( other.d )
{
}

EmailAddressSelectionView::Selection& EmailAddressSelectionView::Selection::operator=( const Selection &other )
{
  if ( this != &other )
    d = other.d;

  return *this;
}

EmailAddressSelectionView::Selection::~Selection()
{
}

bool EmailAddressSelectionView::Selection::isValid() const
{
  return d->mItem.isValid();
}

QString EmailAddressSelectionView::Selection::name() const
{
  return d->mName;
}

QString EmailAddressSelectionView::Selection::email() const
{
  return d->mEmailAddress;
}

Akonadi::Item EmailAddressSelectionView::Selection::item() const
{
  return d->mItem;
}


/**
 * @internal
 */
class EmailAddressSelectionView::Private
{
  public:
    Private( EmailAddressSelectionView *qq, QAbstractItemModel *model )
      : q( qq ), mModel( model )
    {
      init();
    }

    void init();

    EmailAddressSelectionView *q;
    QAbstractItemModel *mModel;
    QLabel *mDescriptionLabel;
    KLineEdit *mSearchLine;
    Akonadi::EntityTreeView *mView;
    LeafExtensionProxyModel *mLeaf;
};

void EmailAddressSelectionView::Private::init()
{
  // setup internal model if needed
  if ( !mModel ) {
    Akonadi::Session *session = new Akonadi::Session( "InternalEmailAddressSelectionViewModel", q );

    Akonadi::ItemFetchScope scope;
    scope.fetchFullPayload( true );
    scope.fetchAttribute<Akonadi::EntityDisplayAttribute>();

    Akonadi::ChangeRecorder *changeRecorder = new Akonadi::ChangeRecorder( q );
    changeRecorder->setSession( session );
    changeRecorder->fetchCollection( true );
    changeRecorder->setItemFetchScope( scope );
    changeRecorder->setCollectionMonitored( Akonadi::Collection::root() );
    changeRecorder->setMimeTypeMonitored( KABC::Addressee::mimeType(), true );
    changeRecorder->setMimeTypeMonitored( KABC::ContactGroup::mimeType(), true );

    Akonadi::ContactsTreeModel *model = new Akonadi::ContactsTreeModel( changeRecorder, q );
//    model->setCollectionFetchStrategy( Akonadi::ContactsTreeModel::InvisibleFetch );

    mModel = model;
  }

  // setup ui
  QVBoxLayout *layout = new QVBoxLayout( q );

  mDescriptionLabel = new QLabel;
  mDescriptionLabel->hide();
  layout->addWidget( mDescriptionLabel );

  QHBoxLayout *searchLayout = new QHBoxLayout;
  layout->addLayout( searchLayout );

  QLabel *label = new QLabel( i18n( "Search:" ) );
  mSearchLine = new KLineEdit;
  label->setBuddy( mSearchLine );
  searchLayout->addWidget( label );
  searchLayout->addWidget( mSearchLine );

  mView = new Akonadi::EntityTreeView;
  layout->addWidget( mView );

  Akonadi::ContactsFilterModel *filter = new Akonadi::ContactsFilterModel( q );
  filter->setSourceModel( mModel );

  mLeaf = new LeafExtensionProxyModel( q );
  mLeaf->setSourceModel( filter );

  mView->setModel( mLeaf );
//  mView->header()->hide();

  q->connect( mSearchLine, SIGNAL( textChanged( const QString& ) ),
              filter, SLOT( setFilterString( const QString& ) ) );

  Control::widgetNeedsAkonadi( q );

  mSearchLine->setFocus();
}


EmailAddressSelectionView::EmailAddressSelectionView( QWidget * parent )
  : QWidget( parent ),
    d( new Private( this, 0 ) )
{
}

EmailAddressSelectionView::EmailAddressSelectionView( QAbstractItemModel *model, QWidget * parent )
  : QWidget( parent ),
    d( new Private( this, model ) )
{
}

EmailAddressSelectionView::~EmailAddressSelectionView()
{
  delete d;
}

EmailAddressSelectionView::Selection::List EmailAddressSelectionView::selectedAddresses() const
{
  Selection::List selections;

  //TODO: extract selection from view

  return selections;
}

void EmailAddressSelectionView::setSelectionMode( QAbstractItemView::SelectionMode mode )
{
  //TODO: remove in favor of QTreeView::setSelectionMode
}

QTreeView* EmailAddressSelectionView::view() const
{
  return d->mView;
}

#include "emailaddressselectionview.moc"
