/*
IM address editor widget for KDE PIM

Copyright 2004,2010 Will Stephenson <wstephenson@kde.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) version 3, or any
later version accepted by the membership of KDE e.V. (or its
successor approved by the membership of KDE e.V.), which shall
act as a proxy defined in Section 6 of version 3 of the license.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library.  If not, see <http://www.gnu.org/licenses/>.
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
