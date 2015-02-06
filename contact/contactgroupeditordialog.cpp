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
#include "contactgroupeditor_p.h"

#include <akonadi/collectioncombobox.h>
#include <akonadi/item.h>
#include <kabc/contactgroup.h>
#include <klocale.h>
#include <klocalizedstring.h>
#include <kglobal.h>
#include <kpushbutton.h>
#include <klineedit.h>

#include <QGridLayout>
#include <QLabel>

using namespace Akonadi;

class ContactGroupEditorDialog::Private
{
  public:
    Private( ContactGroupEditorDialog *qq, ContactGroupEditorDialog::Mode mode )
      : q( qq ), mAddressBookBox( 0 ), mEditor( 0 ), mMode( mode )
    {
    }

    void slotGroupNameChanged( const QString& name )
    {
      bool isValid = !( name.contains( QLatin1Char( '@' ) ) || name.contains( QLatin1Char( '.' ) ) );
      q->button( Ok )->setEnabled( !name.trimmed().isEmpty() && isValid );
      mEditor->groupNameIsValid( isValid );
    }

    void readConfig()
    {
      KConfig config( QLatin1String( "akonadi_contactrc" ) );
      KConfigGroup group( &config, QLatin1String( "ContactGroupEditorDialog" ) );
      const QSize size = group.readEntry( "Size", QSize(470, 400) );
      if ( size.isValid() ) {
        q->resize( size );
      }
    }
    void writeConfig()
    {
      KConfig config( QLatin1String( "akonadi_contactrc" ) );
      KConfigGroup group( &config, QLatin1String( "ContactGroupEditorDialog" ) );
      group.writeEntry( "Size", q->size() );
      group.sync();
    }
    ContactGroupEditorDialog *q;
    CollectionComboBox *mAddressBookBox;
    ContactGroupEditor *mEditor;
    ContactGroupEditorDialog::Mode mMode;
};

ContactGroupEditorDialog::ContactGroupEditorDialog( Mode mode, QWidget *parent )
  : KDialog( parent ), d( new Private( this, mode ) )
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

  connect( d->mEditor, SIGNAL(contactGroupStored(Akonadi::Item)),
           this, SIGNAL(contactGroupStored(Akonadi::Item)) );
  connect( d->mEditor->d->mGui.groupName, SIGNAL(textChanged(QString)),
           this, SLOT(slotGroupNameChanged(QString)) );

  button( Ok )->setEnabled( !d->mEditor->d->mGui.groupName->text().trimmed().isEmpty() );

  d->readConfig();
}

ContactGroupEditorDialog::~ContactGroupEditorDialog()
{
  d->writeConfig();
  delete d;
}

void ContactGroupEditorDialog::setContactGroup( const Akonadi::Item &group )
{
  d->mEditor->loadContactGroup( group );
}

void ContactGroupEditorDialog::setDefaultAddressBook( const Akonadi::Collection &addressbook )
{
  if ( d->mMode == EditMode ) {
    return;
  }

  d->mAddressBookBox->setDefaultCollection( addressbook );
}

ContactGroupEditor* ContactGroupEditorDialog::editor() const
{
  return d->mEditor;
}

void ContactGroupEditorDialog::slotButtonClicked( int button )
{
  if ( button == KDialog::Ok ) {
    if ( d->mAddressBookBox ) {
      d->mEditor->setDefaultAddressBook( d->mAddressBookBox->currentCollection() );
    }

    if ( d->mEditor->saveContactGroup() ) {
      accept();
    }
  } else if ( button == KDialog::Cancel ) {
    reject();
  }
}

#include "moc_contactgroupeditordialog.cpp"
