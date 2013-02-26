/*
    Copyright 2008 Ingo Klöcker <kloecker@kde.org>

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

#include "collectionrequester.h"
#include "collectiondialog.h"

#include <klineedit.h>
#include <klocalizedstring.h>
#include <kpushbutton.h>
#include <kicon.h>
#include <kstandardshortcut.h>

#include <QtCore/QEvent>
#include <QAction>
#include <QApplication>

using namespace Akonadi;

class CollectionRequester::Private
{
  public:
    Private( CollectionRequester *parent )
      : q( parent ),
        edit( 0 ),
        button( 0 ),
        collectionDialog( 0 )
    {
    }

    ~Private()
    {
    }

    void init();

    // slots
    void _k_slotOpenDialog();

    CollectionRequester *q;
    Collection collection;
    KLineEdit *edit;
    KPushButton *button;
    CollectionDialog *collectionDialog;
};


void CollectionRequester::Private::init()
{
  q->setMargin( 0 );

  edit = new KLineEdit( q );
  edit->setReadOnly( true );
  edit->setClickMessage( i18n( "No Folder" ) );
  edit->setClearButtonShown( false );
  edit->setFocusPolicy( Qt::NoFocus );

  button = new KPushButton( q );
  button->setIcon( KIcon( QLatin1String( "document-open" ) ) );
  const int buttonSize = edit->sizeHint().height();
  button->setFixedSize( buttonSize, buttonSize );
  button->setToolTip( i18n( "Open collection dialog" ) );

  q->setSpacing( -1 );

  edit->installEventFilter( q );
  q->setFocusProxy( button );
  q->setFocusPolicy( Qt::StrongFocus );

  q->connect( button, SIGNAL(clicked()), q, SLOT(_k_slotOpenDialog()) );

  QAction *openAction = new QAction( q );
  openAction->setShortcut( KStandardShortcut::Open );
  q->connect( openAction, SIGNAL(triggered(bool)), q, SLOT(_k_slotOpenDialog()) );

  collectionDialog = new CollectionDialog( q );
  collectionDialog->setCaption( i18n( "Select a collection" ) );
  collectionDialog->setSelectionMode( QAbstractItemView::SingleSelection );
}

void CollectionRequester::Private::_k_slotOpenDialog()
{
  CollectionDialog *dlg = collectionDialog;

  if ( dlg->exec() != QDialog::Accepted ) {
    return;
  }

  const Akonadi::Collection collection = dlg->selectedCollection();
  q->setCollection( collection );
  emit q->collectionChanged( collection );
}

CollectionRequester::CollectionRequester( QWidget *parent )
  : KHBox( parent ),
    d( new Private( this ) )
{
  d->init();
}


CollectionRequester::CollectionRequester( const Akonadi::Collection &collection, QWidget *parent )
  : KHBox( parent ),
    d( new Private( this ) )
{
  d->init();
  setCollection( collection );
}


CollectionRequester::~CollectionRequester()
{
  delete d;
}


Collection CollectionRequester::collection() const
{
  return d->collection;
}


void CollectionRequester::setCollection( const Collection& collection )
{
  d->collection = collection;
  d->edit->setText( collection.isValid() ? collection.name() : QString() );
  emit collectionChanged( collection );
}

void CollectionRequester::setMimeTypeFilter( const QStringList &mimeTypes )
{
  if ( d->collectionDialog ) {
    d->collectionDialog->setMimeTypeFilter( mimeTypes );
  }
}

QStringList CollectionRequester::mimeTypeFilter() const
{
  if ( d->collectionDialog ) {
    return d->collectionDialog->mimeTypeFilter();
  } else {
    return QStringList();
  }
}

void CollectionRequester::setAccessRightsFilter( Collection::Rights rights )
{
  if ( d->collectionDialog ) {
    d->collectionDialog->setAccessRightsFilter( rights );
  }
}

Collection::Rights CollectionRequester::accessRightsFilter() const
{
  if ( d->collectionDialog ) {
    return d->collectionDialog->accessRightsFilter();
  } else {
    return Akonadi::Collection::ReadOnly;
  }
}

void CollectionRequester::changeCollectionDialogOptions( CollectionDialog::CollectionDialogOptions options )
{
  if ( d->collectionDialog ) {
    d->collectionDialog->changeCollectionDialogOptions( options );
  }
}

#include "moc_collectionrequester.cpp"
