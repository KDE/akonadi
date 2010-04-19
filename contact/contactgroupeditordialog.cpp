/*
    This file is part of Akonadi Contact.

    Copyright (c) 2007-2009 Tobias Koenig <tokoe@kde.org>

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

#include "contactgroupeditordialog.h"

#include "contactgroupeditor.h"

#include <akonadi/collectioncombobox.h>
#include <akonadi/item.h>
#include <kabc/contactgroup.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <klineedit.h>

#include <QtGui/QGridLayout>
#include <QtGui/QLabel>

using namespace Akonadi;

class ContactGroupEditorDialog::Private
{
  public:
    Private( ContactGroupEditorDialog::Mode mode )
      : mAddressBookBox( 0 ), mMode( mode )
    {
    }

    CollectionComboBox *mAddressBookBox;
    ContactGroupEditor *mEditor;
    ContactGroupEditorDialog::Mode mMode;
};

ContactGroupEditorDialog::ContactGroupEditorDialog( Mode mode, QWidget *parent )
  : KDialog( parent ), d( new Private( mode ) )
{
  KGlobal::locale()->insertCatalog( QLatin1String( "akonadicontact" ) );
  setCaption( mode == CreateMode ? i18n( "New Contact Group" ) : i18n( "Edit Contact Group" ) );
  setButtons( Ok | Cancel );

  // Disable default button, so that finish editing of
  // a member with the Enter key does not close the dialog
  button( Ok )->setAutoDefault( false );
  button( Cancel )->setAutoDefault( false );

  QWidget *mainWidget = new QWidget( this );
  setMainWidget( mainWidget );

  QGridLayout *layout = new QGridLayout( mainWidget );

  d->mEditor = new Akonadi::ContactGroupEditor( mode == CreateMode ?
                                                Akonadi::ContactGroupEditor::CreateMode : Akonadi::ContactGroupEditor::EditMode,
                                                this );

  if ( mode == CreateMode ) {
    QLabel *label = new QLabel( i18n( "Add to:" ), mainWidget );

    d->mAddressBookBox = new CollectionComboBox( mainWidget );
    d->mAddressBookBox->setMimeTypeFilter( QStringList() << KABC::ContactGroup::mimeType() );
    d->mAddressBookBox->setAccessRightsFilter( Collection::CanCreateItem );

    layout->addWidget( label, 0, 0 );
    layout->addWidget( d->mAddressBookBox, 0, 1 );
  }

  layout->addWidget( d->mEditor, 1, 0, 1, 2 );
  layout->setColumnStretch( 1, 1 );

  connect( d->mEditor, SIGNAL( contactGroupStored( const Akonadi::Item& ) ),
           this, SIGNAL( contactGroupStored( const Akonadi::Item& ) ) );
  connect( d->mEditor->groupName(), SIGNAL( textChanged( const QString& ) ),
           this, SLOT( slotGroupNameChanged( const QString& ) ) );
  button( Ok )->setEnabled( !d->mEditor->groupName()->text().isEmpty() );
  setInitialSize( QSize( 470, 400 ) );
}

ContactGroupEditorDialog::~ContactGroupEditorDialog()
{
  delete d;
}

void ContactGroupEditorDialog::slotGroupNameChanged( const QString& name )
{
  button( Ok )->setEnabled( !name.isEmpty() );
}

void ContactGroupEditorDialog::setContactGroup( const Akonadi::Item &group )
{
  d->mEditor->loadContactGroup( group );
}

void ContactGroupEditorDialog::setDefaultAddressBook( const Akonadi::Collection &addressbook )
{
  if ( d->mMode == EditMode )
    return;

  d->mAddressBookBox->setDefaultCollection( addressbook );
}

ContactGroupEditor* ContactGroupEditorDialog::editor() const
{
  return d->mEditor;
}

void ContactGroupEditorDialog::slotButtonClicked( int button )
{
  if ( button == KDialog::Ok ) {
    if ( d->mAddressBookBox )
      d->mEditor->setDefaultAddressBook( d->mAddressBookBox->currentCollection() );

    if ( d->mEditor->saveContactGroup() )
      accept();
  } else if ( button == KDialog::Cancel ) {
    reject();
  }
}

#include "contactgroupeditordialog.moc"
