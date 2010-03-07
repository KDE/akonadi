/*
  IM address editor widget for KAddressBook

  Copyright (c) 2004 Will Stephenson   <lists@stevello.free-online.co.uk>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "imeditordialog.h"

#include "imdelegate.h"

#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtGui/QGridLayout>
#include <QtGui/QPushButton>
#include <QtGui/QTreeView>

#include <klocale.h>
#include <kmessagebox.h>

IMEditorDialog::IMEditorDialog( QWidget *parent )
  : KDialog( parent )
{
  setCaption( i18n( "Edit Instant Messaging Address" ) );
  setButtons( Ok | Cancel );
  setDefaultButton( Ok );

  QWidget *widget = new QWidget( this );
  setMainWidget( widget );

  QGridLayout *layout = new QGridLayout( widget );

  mAddButton = new QPushButton( i18n( "Add" ) );
  mRemoveButton = new QPushButton( i18n( "Remove" ) );
  mStandardButton = new QPushButton( i18n( "Set as Standard" ) );

  mView = new QTreeView;
  mView->setRootIsDecorated( false );

  layout->addWidget( mView, 0, 0, 4, 1 );
  layout->addWidget( mAddButton, 0, 1 );
  layout->addWidget( mRemoveButton, 1, 1 );
  layout->addWidget( mStandardButton, 2, 1 );

  connect( mAddButton, SIGNAL( clicked() ), SLOT( slotAdd() ) );
  connect( mRemoveButton, SIGNAL( clicked() ), SLOT( slotRemove() ) );
  connect( mStandardButton, SIGNAL( clicked()), SLOT( slotSetStandard() ) );

  mRemoveButton->setEnabled( false );
  mStandardButton->setEnabled( false );

  mModel = new IMModel( this );

  mView->setModel( mModel );
  mView->setItemDelegate( new IMDelegate( this ) );

  connect( mView->selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ),
           this, SLOT( slotUpdateButtons() ) );
}

void IMEditorDialog::setAddresses( const IMAddress::List &addresses )
{
  mModel->setAddresses( addresses );
}

IMAddress::List IMEditorDialog::addresses() const
{
  return mModel->addresses();
}

void IMEditorDialog::slotAdd()
{
  mModel->insertRow( mModel->rowCount() );
}

void IMEditorDialog::slotRemove()
{
  const QModelIndex currentIndex = mView->currentIndex();
  if ( !currentIndex.isValid() )
    return;

  if ( KMessageBox::warningContinueCancel( this,
                                           i18nc( "Instant messaging", "Do you really want to delete the selected address?" ),
                                           i18n( "Confirm Delete" ), KStandardGuiItem::del() ) != KMessageBox::Continue ) {
    return;
  }

  mModel->removeRow( currentIndex.row() );
}

void IMEditorDialog::slotSetStandard()
{
  const QModelIndex currentIndex = mView->currentIndex();
  if ( !currentIndex.isValid() )
    return;

  // set current index as preferred and all other as non-preferred
  for ( int i = 0; i < mModel->rowCount(); ++i ) {
    const QModelIndex index = mModel->index( i, 0 );
    mModel->setData( index, (index.row() == currentIndex.row()), IMModel::IsPreferredRole );
  }
}

void IMEditorDialog::slotUpdateButtons()
{
  const QModelIndex currentIndex = mView->currentIndex();

  mRemoveButton->setEnabled( currentIndex.isValid() );
  mStandardButton->setEnabled( currentIndex.isValid() );
}

#include "imeditordialog.moc"
