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

#include "contacteditordialog.h"

#include "contacteditor.h"

#include <akonadi/collectioncombobox.h>
#include <akonadi/item.h>

#include <kabc/addressee.h>

#include <klocale.h>
#include <klocalizedstring.h>
#include <kglobal.h>

#include <QGridLayout>
#include <QLabel>

using namespace Akonadi;

class ContactEditorDialog::Private
{
  public:
    Private( ContactEditorDialog::Mode mode, ContactEditorDialog::DisplayMode displaymode, AbstractContactEditorWidget *editorWidget,
             ContactEditorDialog *parent )
      : q( parent ), mAddressBookBox( 0 ), mMode( mode )
    {
      KLocalizedString::insertCatalog( QLatin1String( "akonadicontact" ) );
      q->setCaption( mode == ContactEditorDialog::CreateMode ? i18n( "New Contact" ) : i18n( "Edit Contact" ) );
      q->setButtons( ContactEditorDialog::Ok | ContactEditorDialog::Cancel );

      QWidget *mainWidget = new QWidget( q );
      q->setMainWidget( mainWidget );

      QGridLayout *layout = new QGridLayout( mainWidget );

      if ( editorWidget ) {
        mEditor = new ContactEditor( mode == ContactEditorDialog::CreateMode ? ContactEditor::CreateMode : ContactEditor::EditMode, editorWidget, q );
      } else {
        mEditor = new ContactEditor( mode == ContactEditorDialog::CreateMode ? ContactEditor::CreateMode : ContactEditor::EditMode, displaymode == ContactEditorDialog::FullMode ? ContactEditor::FullMode : ContactEditor::VCardMode, q );
      }

      if ( mode == ContactEditorDialog::CreateMode ) {
        QLabel *label = new QLabel( i18n( "Add to:" ), mainWidget );

        mAddressBookBox = new CollectionComboBox( mainWidget );
        mAddressBookBox->setMimeTypeFilter( QStringList() << KABC::Addressee::mimeType() );
        mAddressBookBox->setAccessRightsFilter( Collection::CanCreateItem );

        layout->addWidget( label, 0, 0 );
        layout->addWidget( mAddressBookBox, 0, 1 );
      }

      layout->addWidget( mEditor, 1, 0, 1, 2 );
      layout->setColumnStretch( 1, 1 );

      connect( mEditor, SIGNAL(contactStored(Akonadi::Item)),
               q, SIGNAL(contactStored(Akonadi::Item)) );

      connect( mEditor, SIGNAL(error(QString)),
               q, SIGNAL(error(QString)) );

      connect( q, SIGNAL(okClicked()), q, SLOT(slotOkClicked()) );
      connect( q, SIGNAL(cancelClicked()), q, SLOT(slotCancelClicked()) );
      connect( mEditor, SIGNAL(finished()), q, SLOT(slotFinish()) );

      q->setInitialSize( QSize( 800, 500 ) );
    }

    void slotOkClicked()
    {
      if ( mAddressBookBox ) {
        mEditor->setDefaultAddressBook( mAddressBookBox->currentCollection() );
      }
      mEditor->saveContactInAddressBook();
    }

    void slotFinish()
    {
        q->KDialog::accept();
    }

    void slotCancelClicked()
    {
      q->reject();
    }

    ContactEditorDialog *q;
    CollectionComboBox *mAddressBookBox;
    ContactEditorDialog::Mode mMode;
    ContactEditor *mEditor;
};

ContactEditorDialog::ContactEditorDialog( Mode mode, QWidget *parent )
  : KDialog( parent ), d( new Private( mode, FullMode, 0, this ) )
{
}

ContactEditorDialog::ContactEditorDialog( Mode mode, AbstractContactEditorWidget *editorWidget, QWidget *parent )
  : KDialog( parent ), d( new Private( mode, FullMode, editorWidget, this ) )
{
}

ContactEditorDialog::ContactEditorDialog( Mode mode, DisplayMode displayMode, QWidget *parent )
  : KDialog( parent ), d( new Private( mode, displayMode, 0, this ) )
{
}

ContactEditorDialog::~ContactEditorDialog()
{
  delete d;
}

void ContactEditorDialog::setContact( const Akonadi::Item &contact )
{
  d->mEditor->loadContact( contact );
}

void ContactEditorDialog::setDefaultAddressBook( const Akonadi::Collection &addressbook )
{
  if ( d->mMode == EditMode ) {
    return;
  }

  d->mAddressBookBox->setDefaultCollection( addressbook );
}

ContactEditor* ContactEditorDialog::editor() const
{
  return d->mEditor;
}


void ContactEditorDialog::accept()
{
    //Nothing
}

#include "moc_contacteditordialog.cpp"
