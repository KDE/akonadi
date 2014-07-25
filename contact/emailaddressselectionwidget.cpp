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

#include "emailaddressselectionwidget.h"

#include "emailaddressselection_p.h"
#include "emailaddressselectionproxymodel_p.h"

#include <akonadi/changerecorder.h>
#include <akonadi/contact/contactsfilterproxymodel.h>
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
#include <klocalizedstring.h>
#include <kglobal.h>

#include <QtCore/QTimer>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QVBoxLayout>

using namespace Akonadi;

/**
 * @internal
 */
class SearchLineEdit : public KLineEdit
{
  public:
    SearchLineEdit( QWidget *receiver, QWidget *parent = 0 )
      : KLineEdit( parent ), mReceiver( receiver )
    {
        setClearButtonShown( true );
    }

  protected:
    virtual void keyPressEvent( QKeyEvent *event )
    {
      if ( event->key() == Qt::Key_Down ) {
        QMetaObject::invokeMethod( mReceiver, "setFocus" );
      }

      KLineEdit::keyPressEvent( event );
    }

  private:
    QWidget *mReceiver;
};

/**
 * @internal
 */
class EmailAddressSelectionWidget::Private
{
  public:
    Private( bool showOnlyContactWithEmail, EmailAddressSelectionWidget *qq, QAbstractItemModel *model )
      : q( qq ), mModel( model ), mShowOnlyContactWithEmail(showOnlyContactWithEmail)
    {
      init();
    }

    void init();

    EmailAddressSelectionWidget *q;
    QAbstractItemModel *mModel;
    QLabel *mDescriptionLabel;
    SearchLineEdit *mSearchLine;
    Akonadi::EntityTreeView *mView;
    EmailAddressSelectionProxyModel *mSelectionModel;
    bool mShowOnlyContactWithEmail;
};

void EmailAddressSelectionWidget::Private::init()
{
  KGlobal::locale()->insertCatalog( QLatin1String( "akonadicontact" ) );
  // setup internal model if needed
  if ( !mModel ) {
    Akonadi::Session *session = new Akonadi::Session( "InternalEmailAddressSelectionWidgetModel", q );

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

  mView = new Akonadi::EntityTreeView;

  QLabel *label = new QLabel( i18nc( "@label Search in a list of contacts", "Search:" ) );
  mSearchLine = new SearchLineEdit( mView );
  label->setBuddy( mSearchLine );
  searchLayout->addWidget( label );
  searchLayout->addWidget( mSearchLine );

#ifndef QT_NO_DRAGANDDROP
  mView->setDragDropMode( QAbstractItemView::NoDragDrop );
#endif
  layout->addWidget( mView );

  Akonadi::ContactsFilterProxyModel *filter = new Akonadi::ContactsFilterProxyModel( q );
  if (mShowOnlyContactWithEmail)
      filter->setFilterFlags( ContactsFilterProxyModel::HasEmail );
  filter->setExcludeVirtualCollections( true );
  filter->setSourceModel( mModel );

  mSelectionModel = new EmailAddressSelectionProxyModel( q );
  mSelectionModel->setSourceModel( filter );

  mView->setModel( mSelectionModel );
  mView->header()->hide();

  q->connect( mSearchLine, SIGNAL(textChanged(QString)),
              filter, SLOT(setFilterString(QString)) );

  q->connect( mView, SIGNAL(doubleClicked(Akonadi::Item)),
              q, SIGNAL(doubleClicked()));
  Control::widgetNeedsAkonadi( q );

  mSearchLine->setFocus();

  QTimer::singleShot( 1000, mView, SLOT(expandAll()) );
}

EmailAddressSelectionWidget::EmailAddressSelectionWidget( QWidget * parent )
  : QWidget( parent ),
    d( new Private( true, this, 0 ) )
{
}

EmailAddressSelectionWidget::EmailAddressSelectionWidget( QAbstractItemModel *model, QWidget * parent )
  : QWidget( parent ),
    d( new Private( true, this, model ) )
{
}

EmailAddressSelectionWidget::EmailAddressSelectionWidget( bool showOnlyContactWithEmail, QAbstractItemModel *model, QWidget * parent )
  : QWidget( parent ),
    d( new Private( showOnlyContactWithEmail ,this, model ) )
{
}

EmailAddressSelectionWidget::~EmailAddressSelectionWidget()
{
  delete d;
}

EmailAddressSelection::List EmailAddressSelectionWidget::selectedAddresses() const
{
  EmailAddressSelection::List selections;

  if ( !d->mView->selectionModel() ) {
    return selections;
  }

  const QModelIndexList selectedRows = d->mView->selectionModel()->selectedRows( 0 );
  foreach ( const QModelIndex &index, selectedRows ) {
    EmailAddressSelection selection;
    selection.d->mName = index.data( EmailAddressSelectionProxyModel::NameRole ).toString();
    selection.d->mEmailAddress = index.data( EmailAddressSelectionProxyModel::EmailAddressRole ).toString();
    selection.d->mItem = index.data( ContactsTreeModel::ItemRole ).value<Akonadi::Item>();

    if ( d->mShowOnlyContactWithEmail ) {
        if ( !selection.d->mEmailAddress.isEmpty() ) {
            selections << selection;
        }
    } else {
        selections << selection;
    }
  }

  return selections;
}

KLineEdit* EmailAddressSelectionWidget::searchLineEdit() const
{
  return d->mSearchLine;
}

QTreeView* EmailAddressSelectionWidget::view() const
{
  return d->mView;
}

