/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "subscriptiondialog_p.h"

#include "subscriptionmodel_p.h"
#include "subscriptionjob_p.h"
#include "control.h"

#include <kdebug.h>

#include <QtGui/QBoxLayout>

#include <klocale.h>

#ifndef KDEPIM_MOBILE_UI
#include <krecursivefilterproxymodel.h>
#include <recursivecollectionfilterproxymodel.h>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QTreeView>
#include <klineedit.h>
#else
#include <QtGui/QListView>
#include <QtGui/QSortFilterProxyModel>
#include "kdescendantsproxymodel_p.h"

class CheckableFilterProxyModel : public QSortFilterProxyModel
{
public:
  CheckableFilterProxyModel( QObject *parent = 0 )
    : QSortFilterProxyModel( parent ) { }

protected:
  /*reimp*/ bool filterAcceptsRow( int sourceRow, const QModelIndex &sourceParent ) const
  {
    QModelIndex sourceIndex = sourceModel()->index( sourceRow, 0, sourceParent );
    return sourceModel()->flags( sourceIndex ) & Qt::ItemIsUserCheckable;
  }
};
#endif

using namespace Akonadi;

/**
 * @internal
 */
class SubscriptionDialog::Private
{
  public:
    Private( SubscriptionDialog *parent ) : q( parent ) {}

    void done()
    {
      SubscriptionJob *job = new SubscriptionJob( q );
      job->subscribe( model->subscribed() );
      job->unsubscribe( model->unsubscribed() );
      connect( job, SIGNAL( result( KJob* ) ), q, SLOT( subscriptionResult( KJob* ) ) );
    }

    void subscriptionResult( KJob *job )
    {
      if ( job->error() ) {
        // TODO
        kWarning() << job->errorString();
      }
      q->deleteLater();
    }

    void modelLoaded()
    {
      collectionView->setEnabled( true );
#ifndef KDEPIM_MOBILE_UI
      collectionView->expandAll();
#endif
      q->enableButtonOk( true );
    }

    SubscriptionDialog* q;
#ifndef KDEPIM_MOBILE_UI
    QTreeView *collectionView;
#else
    QListView *collectionView;
#endif
    SubscriptionModel* model;
};

SubscriptionDialog::SubscriptionDialog(QWidget * parent) :
    KDialog( parent ),
    d( new Private( this ) )
{
  init();
}

SubscriptionDialog::SubscriptionDialog(const QString& mimetype, QWidget * parent) :
    KDialog( parent ),
    d( new Private( this ) )
{
  init( mimetype );
}

void SubscriptionDialog::init( const QString& mimetype )
{
  enableButtonOk( false );
  setCaption( i18n( "Local Subscriptions" ) );
  QWidget *mainWidget = new QWidget( this );
  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainWidget->setLayout( mainLayout );
  setMainWidget( mainWidget );

  d->model = new SubscriptionModel( this );

#ifndef KDEPIM_MOBILE_UI
  KRecursiveFilterProxyModel *filterTreeViewModel = new KRecursiveFilterProxyModel( this );
  filterTreeViewModel->setDynamicSortFilter( true );
  filterTreeViewModel->setSourceModel( d->model );
  filterTreeViewModel->setFilterCaseSensitivity( Qt::CaseInsensitive );

  RecursiveCollectionFilterProxyModel *filterRecursiveCollectionFilter
      = new Akonadi::RecursiveCollectionFilterProxyModel( this );
  if ( !mimetype.isEmpty() )
    filterRecursiveCollectionFilter->addContentMimeTypeInclusionFilter( mimetype );

  filterRecursiveCollectionFilter->setSourceModel( filterTreeViewModel );

  QHBoxLayout *filterBarLayout = new QHBoxLayout;

  filterBarLayout->addWidget( new QLabel( i18n( "Search:" ) ) );

  KLineEdit *lineEdit = new KLineEdit( mainWidget );
  connect( lineEdit, SIGNAL( textChanged( const QString& ) ),
           filterTreeViewModel, SLOT( setFilterFixedString( const QString& ) ) );
  filterBarLayout->addWidget( lineEdit );
  mainLayout->addLayout( filterBarLayout );

  d->collectionView = new QTreeView( mainWidget );
  d->collectionView->header()->hide();
  d->collectionView->setModel( filterRecursiveCollectionFilter );
  mainLayout->addWidget( d->collectionView );
#else
  KDescendantsProxyModel *flatModel = new KDescendantsProxyModel( this );
  flatModel->setDisplayAncestorData( true );
  flatModel->setAncestorSeparator( QLatin1String( "/" ) );
  flatModel->setSourceModel( d->model );

  CheckableFilterProxyModel *checkableModel = new CheckableFilterProxyModel( this );
  checkableModel->setSourceModel( flatModel );

  d->collectionView = new QListView( mainWidget );

  d->collectionView->setModel( checkableModel );
  mainLayout->addWidget( d->collectionView );
#endif

  connect( d->model, SIGNAL( loaded() ), SLOT( modelLoaded() ) );
  connect( this, SIGNAL( okClicked() ), SLOT( done() ) );
  connect( this, SIGNAL( cancelClicked() ), SLOT( deleteLater() ) );
  Control::widgetNeedsAkonadi( mainWidget );
}

SubscriptionDialog::~ SubscriptionDialog()
{
  delete d;
}

#include "subscriptiondialog_p.moc"
