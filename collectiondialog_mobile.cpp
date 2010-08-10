/*
    Copyright 2010 Tobias Koenig <tokoe@kde.org>

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

#include "collectiondialog.h"

#include <akonadi/collectioncombobox.h>
#include <akonadi/collectioncreatejob.h>
#include <akonadi/collectionutils_p.h>

#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>

#include <KLocale>
#include <KInputDialog>
#include <KMessageBox>

using namespace Akonadi;

class CollectionDialog::Private
{
  public:
    Private( QAbstractItemModel *customModel, CollectionDialog *parent, CollectionDialogOptions options )
      : mParent( parent ),
        mSelectionMode( QAbstractItemView::SingleSelection )
    {
      // setup GUI
      QWidget *widget = mParent->mainWidget();
      QVBoxLayout *layout = new QVBoxLayout( widget );

      changeCollectionDialogOptions( options );

      mTextLabel = new QLabel;
      layout->addWidget( mTextLabel );
      mTextLabel->hide();

      mCollectionComboBox = new CollectionComboBox( customModel, widget );
      layout->addWidget( mCollectionComboBox );
      mParent->connect( mCollectionComboBox, SIGNAL( currentIndexChanged( int ) ), SLOT( slotSelectionChanged() ) );

      mParent->enableButton( KDialog::Ok, false );
    }

    ~Private()
    {
    }

    void slotCollectionAvailable( const QModelIndex& )
    {
    }

    CollectionDialog *mParent;

    QLabel *mTextLabel;
    CollectionComboBox *mCollectionComboBox;
    QAbstractItemView::SelectionMode mSelectionMode;
    bool mAllowToCreateNewChildCollection;

    void slotSelectionChanged();
    void slotAddChildCollection();
    void slotCollectionCreationResult( KJob* job );
    bool canCreateCollection( const Akonadi::Collection &parentCollection ) const;
    void changeCollectionDialogOptions( CollectionDialogOptions options );

};

void CollectionDialog::Private::slotSelectionChanged()
{
  mParent->enableButton( KDialog::Ok, mCollectionComboBox->count() > 0 );
  if ( mAllowToCreateNewChildCollection ) {
    const Akonadi::Collection parentCollection = mParent->selectedCollection();
    const bool canCreateChildCollections = canCreateCollection( parentCollection );
    const bool isVirtual = Akonadi::CollectionUtils::isVirtual( parentCollection );

    mParent->enableButton( KDialog::User1, (canCreateChildCollections && !isVirtual) );
    if ( parentCollection.isValid() ) {
      const bool canCreateItems = (parentCollection.rights() & Akonadi::Collection::CanCreateItem);
      mParent->enableButton( KDialog::Ok, canCreateItems );
    }
  }
}

void CollectionDialog::Private::changeCollectionDialogOptions( CollectionDialogOptions options )
{
  mAllowToCreateNewChildCollection = ( options & AllowToCreateNewChildCollection );
  if ( mAllowToCreateNewChildCollection ) {
    mParent->setButtons( Ok | Cancel | User1 );
    mParent->setButtonGuiItem( User1, KGuiItem( i18n( "&New Subfolder..." ), QLatin1String( "folder-new" ),
                                                i18n( "Create a new subfolder under the currently selected folder" ) ) );
    mParent->enableButton( KDialog::User1, false );
    connect( mParent, SIGNAL( user1Clicked() ), mParent, SLOT( slotAddChildCollection() ) );
  }
}



bool CollectionDialog::Private::canCreateCollection( const Akonadi::Collection &parentCollection ) const
{
  if ( !parentCollection.isValid() )
    return false;

  if ( ( parentCollection.rights() & Akonadi::Collection::CanCreateCollection ) ) {
    const QStringList dialogMimeTypeFilter = mParent->mimeTypeFilter();
    const QStringList parentCollectionMimeTypes = parentCollection.contentMimeTypes();
    Q_FOREACH ( const QString& mimetype, dialogMimeTypeFilter ) {
      if ( parentCollectionMimeTypes.contains( mimetype ) )
        return true;
    }
    return true;
  }
  return false;
}


void CollectionDialog::Private::slotAddChildCollection()
{
  const Akonadi::Collection parentCollection = mParent->selectedCollection();
  if ( canCreateCollection( parentCollection ) ) {
    const QString name = KInputDialog::getText( i18nc( "@title:window", "New Folder" ),
                                                i18nc( "@label:textbox, name of a thing", "Name" ),
                                                QString(), 0, mParent );
    if ( name.isEmpty() )
      return;

    Akonadi::Collection collection;
    collection.setName( name );
    collection.setParentCollection( parentCollection );
    Akonadi::CollectionCreateJob *job = new Akonadi::CollectionCreateJob( collection );
    connect( job, SIGNAL( result( KJob* ) ), mParent, SLOT( slotCollectionCreationResult( KJob* ) ) );
  }
}

void CollectionDialog::Private::slotCollectionCreationResult( KJob* job )
{
  if ( job->error() ) {
    KMessageBox::error( mParent, i18n( "Could not create folder: %1", job->errorString() ),
                        i18n( "Folder creation failed" ) );
  }
}



CollectionDialog::CollectionDialog( QWidget *parent )
  : KDialog( parent ),
    d( new Private( 0, this, CollectionDialog::None ) )
{
}

CollectionDialog::CollectionDialog( QAbstractItemModel *model, QWidget *parent )
  : KDialog( parent ),
    d( new Private( model, this, CollectionDialog::None ) )
{
}

CollectionDialog::CollectionDialog( CollectionDialogOptions options, QAbstractItemModel *model, QWidget *parent )
  : KDialog( parent ),
    d( new Private( model, this, options ) )
{
}


CollectionDialog::~CollectionDialog()
{
  delete d;
}

Akonadi::Collection CollectionDialog::selectedCollection() const
{
  return d->mCollectionComboBox->currentCollection();
}

Akonadi::Collection::List CollectionDialog::selectedCollections() const
{
  return (Collection::List() << d->mCollectionComboBox->currentCollection());
}

void CollectionDialog::setMimeTypeFilter( const QStringList &mimeTypes )
{
  d->mCollectionComboBox->setMimeTypeFilter( mimeTypes );
}

QStringList CollectionDialog::mimeTypeFilter() const
{
  return d->mCollectionComboBox->mimeTypeFilter();
}

void CollectionDialog::setAccessRightsFilter( Collection::Rights rights )
{
  d->mCollectionComboBox->setAccessRightsFilter( rights );
}

Collection::Rights CollectionDialog::accessRightsFilter() const
{
  return d->mCollectionComboBox->accessRightsFilter();
}

void CollectionDialog::setDescription( const QString &text )
{
  d->mTextLabel->setText( text );
  d->mTextLabel->show();
}

void CollectionDialog::setDefaultCollection( const Collection &collection )
{
  d->mCollectionComboBox->setDefaultCollection( collection );
}

void CollectionDialog::setSelectionMode( QAbstractItemView::SelectionMode mode )
{
  d->mSelectionMode = mode;
}

QAbstractItemView::SelectionMode CollectionDialog::selectionMode() const
{
  return d->mSelectionMode;
}

void CollectionDialog::changeCollectionDialogOptions( CollectionDialogOptions options )
{
  d->changeCollectionDialogOptions( options );
}

#include "collectiondialog.moc"
