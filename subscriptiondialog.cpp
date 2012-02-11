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

#include "control.h"
#include "recursivecollectionfilterproxymodel.h"
#include "subscriptionjob_p.h"
#include "subscriptionmodel_p.h"

#include <kdebug.h>

#include <QtGui/QBoxLayout>

#include <klocale.h>

#ifndef KDEPIM_MOBILE_UI
#include <klineedit.h>
#include <krecursivefilterproxymodel.h>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QTreeView>
#include <QtGui/QCheckBox>
#else
#include <kdescendantsproxymodel.h>
#include <QtGui/QListView>
#include <QtGui/QSortFilterProxyModel>

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
      connect( job, SIGNAL(result(KJob*)), q, SLOT(subscriptionResult(KJob*)) );
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
    void slotSetPattern(const QString &text)
    {
      filterRecursiveCollectionFilter->setSearchPattern( text );
    }
    void slotSetIncludeCheckedOnly(bool checked)
    {
      filterRecursiveCollectionFilter->setIncludeCheckedOnly( checked );
    }

    SubscriptionDialog* q;
#ifndef KDEPIM_MOBILE_UI
    QTreeView *collectionView;
#else
    QListView *collectionView;
#endif
    SubscriptionModel* model;
    RecursiveCollectionFilterProxyModel *filterRecursiveCollectionFilter;

};

SubscriptionDialog::SubscriptionDialog(QWidget * parent) :
    KDialog( parent ),
    d( new Private( this ) )
{
  init( QStringList() );
}

SubscriptionDialog::SubscriptionDialog(const QStringList& mimetypes, QWidget * parent) :
    KDialog( parent ),
    d( new Private( this ) )
{
  init( mimetypes );
}

void SubscriptionDialog::init( const QStringList &mimetypes )
{
  enableButtonOk( false );
  setCaption( i18n( "Local Subscriptions" ) );
  QWidget *mainWidget = new QWidget( this );
  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainWidget->setLayout( mainLayout );
  setMainWidget( mainWidget );

  d->model = new SubscriptionModel( this );

#ifndef KDEPIM_MOBILE_UI
  d->filterRecursiveCollectionFilter
      = new Akonadi::RecursiveCollectionFilterProxyModel( this );
  d->filterRecursiveCollectionFilter->setDynamicSortFilter( true );
  d->filterRecursiveCollectionFilter->setSourceModel( d->model );
  d->filterRecursiveCollectionFilter->setFilterCaseSensitivity( Qt::CaseInsensitive );
  if ( !mimetypes.isEmpty() ) {
    d->filterRecursiveCollectionFilter->addContentMimeTypeInclusionFilters( mimetypes );
  }

  d->collectionView = new QTreeView( mainWidget );
  d->collectionView->setEditTriggers( QAbstractItemView::NoEditTriggers );
  d->collectionView->header()->hide();
  d->collectionView->setModel( d->filterRecursiveCollectionFilter );


  QHBoxLayout *filterBarLayout = new QHBoxLayout;

  filterBarLayout->addWidget( new QLabel( i18n( "Search:" ) ) );

  KLineEdit *lineEdit = new KLineEdit( mainWidget );
  lineEdit->setClearButtonShown(true);
  lineEdit->setFocus();
  connect( lineEdit, SIGNAL(textChanged(QString)),
           this, SLOT(slotSetPattern(QString)) );
  filterBarLayout->addWidget( lineEdit );
  QCheckBox *checkBox = new QCheckBox( i18n("Subscribed only"), mainWidget );
  connect( checkBox, SIGNAL(clicked(bool)),
           this, SLOT(slotSetIncludeCheckedOnly(bool)) );
  filterBarLayout->addWidget( checkBox );

  mainLayout->addLayout( filterBarLayout );
  mainLayout->addWidget( d->collectionView );
#else

  d->filterRecursiveCollectionFilter
      = new Akonadi::RecursiveCollectionFilterProxyModel( this );
  if ( !mimetypes.isEmpty() )
    d->filterRecursiveCollectionFilter->addContentMimeTypeInclusionFilters( mimetypes );

  d->filterRecursiveCollectionFilter->setSourceModel( d->model );

  KDescendantsProxyModel *flatModel = new KDescendantsProxyModel( this );
  flatModel->setDisplayAncestorData( true );
  flatModel->setAncestorSeparator( QLatin1String( "/" ) );
  flatModel->setSourceModel( d->filterRecursiveCollectionFilter );

  CheckableFilterProxyModel *checkableModel = new CheckableFilterProxyModel( this );
  checkableModel->setSourceModel( flatModel );

  d->collectionView = new QListView( mainWidget );

  d->collectionView->setModel( checkableModel );
  mainLayout->addWidget( d->collectionView );
#endif

  connect( d->model, SIGNAL(loaded()), SLOT(modelLoaded()) );
  connect( this, SIGNAL(okClicked()), SLOT(done()) );
  connect( this, SIGNAL(cancelClicked()), SLOT(deleteLater()) );
  Control::widgetNeedsAkonadi( mainWidget );
}

SubscriptionDialog::~ SubscriptionDialog()
{
  delete d;
}


#include "subscriptiondialog_p.moc"
