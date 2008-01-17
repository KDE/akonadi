/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "collectionpropertiesdialog.h"

#include "collectiongeneralpropertiespage.h"
#include "collectionmodifyjob.h"

#include <kdebug.h>

#include <QtGui/QBoxLayout>
#include <QtGui/QTabWidget>

using namespace Akonadi;

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionGeneralPropertiesPageFactory, CollectionGeneralPropertiesPage)

// FIXME: we leak d->pages!

class CollectionPropertiesDialog::Private
{
  public:
    Private( CollectionPropertiesDialog *parent ) : q( parent )
    {
      if ( pages.isEmpty() )
        registerBuiltinPages();
    }

    static void registerBuiltinPages()
    {
      pages.append( new CollectionGeneralPropertiesPageFactory() );
    }

    void save()
    {
      for ( int i = 0; i < tabWidget->count(); ++i ) {
        CollectionPropertiesPage *page = static_cast<CollectionPropertiesPage*>( tabWidget->widget( i ) );
        page->save( collection );
      }

      CollectionModifyJob *job = new CollectionModifyJob( collection, q );
      connect( job, SIGNAL(result(KJob*)), q, SLOT(saveResult(KJob*)) );
      job->setName( collection.name() );
      job->setContentTypes( collection.contentTypes() );
      job->setCachePolicy( collection.cachePolicyId() );
      foreach ( CollectionAttribute *attr, collection.attributes() )
        job->setAttribute( attr );
      // TODO complete me
    }

    void saveResult( KJob *job )
    {
      if ( job->error() ) {
        // TODO
        kWarning() << job->errorString();
      }
      q->deleteLater();
    }

    Collection collection;
    static QList<CollectionPropertiesPageFactory*> pages;
    QTabWidget* tabWidget;
    CollectionPropertiesDialog *q;
};

QList<CollectionPropertiesPageFactory*> CollectionPropertiesDialog::Private::pages;

CollectionPropertiesDialog::CollectionPropertiesDialog(const Collection & collection, QWidget * parent) :
    KDialog( parent ),
    d( new Private( this ) )
{
  d->collection = collection;

  QBoxLayout *layout = new QHBoxLayout( mainWidget() );
  layout->setMargin( 0 );
  d->tabWidget = new QTabWidget( mainWidget() );
  layout->addWidget( d->tabWidget );

  foreach ( CollectionPropertiesPageFactory *factory, d->pages ) {
    CollectionPropertiesPage *page = factory->createWidget( d->tabWidget );
    if ( page->canHandle( d->collection ) ) {
      d->tabWidget->addTab( page, page->pageTitle() );
      page->load( d->collection );
    } else {
      delete page;
    }
  }

  connect( this, SIGNAL(okClicked()), SLOT(save()) );
  connect( this, SIGNAL(cancelClicked()), SLOT(deleteLater()) );
}

CollectionPropertiesDialog::~CollectionPropertiesDialog()
{
  delete d;
}

void CollectionPropertiesDialog::registerPage(CollectionPropertiesPageFactory * factory)
{
  if ( Private::pages.isEmpty() )
    Private::registerBuiltinPages();
  Private::pages.append( factory );
}

#include "collectionpropertiesdialog.moc"
